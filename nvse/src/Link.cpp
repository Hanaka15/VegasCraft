#include "Link.h"

namespace vegascraft
{
	bool Link::Create()
	{
		Close();
		mapping_ = ::CreateFileMappingW(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE,
			static_cast<DWORD>(proto::kMappingBytes >> 32),
			static_cast<DWORD>(proto::kMappingBytes & 0xFFFFFFFFu),
			proto::kMappingName);
		if (!mapping_) {
			return false;
		}
		view_ = ::MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, static_cast<SIZE_T>(proto::kMappingBytes));
		if (!view_) {
			::CloseHandle(mapping_);
			mapping_ = nullptr;
			return false;
		}
		base_ = static_cast<std::uint8_t*>(view_);
		std::memset(base_, 0, static_cast<size_t>(proto::kMappingBytes));

		auto* hdr = At<proto::Header>(proto::kOffHeader);
		hdr->magic = proto::kMagic;
		hdr->version = proto::kVersion;
		hdr->hostPid = ::GetCurrentProcessId();
		hdr->mcPid = 0;
		hdr->hostHeartbeatMs = ::GetTickCount64();
		hdr->mcHeartbeatMs = 0;
		return true;
	}

	void Link::Close()
	{
		if (view_) {
			::UnmapViewOfFile(view_);
			view_ = nullptr;
			base_ = nullptr;
		}
		if (mapping_) {
			::CloseHandle(mapping_);
			mapping_ = nullptr;
		}
	}

	void Link::Heartbeat()
	{
		if (!base_) {
			return;
		}
		At<proto::Header>(proto::kOffHeader)->hostHeartbeatMs = ::GetTickCount64();
	}

	void Link::WriteHostState(const proto::HostState& state)
	{
		if (!base_) {
			return;
		}
		++hostSeq_;
		auto* dst = At<proto::HostState>(proto::kOffHostState);
		dst->seq = hostSeq_ * 2u - 1u;
		std::memcpy(reinterpret_cast<std::uint8_t*>(dst) + 4,
			reinterpret_cast<const std::uint8_t*>(&state) + 4,
			sizeof(proto::HostState) - 4);
		dst->seq = hostSeq_ * 2u;
	}

	bool Link::ReadMcState(proto::McState& out) const
	{
		if (!base_) {
			return false;
		}
		const auto* src = At<proto::McState>(proto::kOffMcState);
		for (int i = 0; i < 8; ++i) {
			const std::uint32_t s0 = src->seq;
			if (s0 & 1u) {
				continue;
			}
			std::memcpy(&out, src, sizeof(out));
			if (src->seq == s0) {
				return true;
			}
		}
		return false;
	}
}
