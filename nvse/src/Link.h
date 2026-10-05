#pragma once

#include "PCH.h"

namespace vegascraft
{
	class Link
	{
	public:
		bool Create();
		void Close();
		bool IsOpen() const { return view_ != nullptr; }

		void Heartbeat();
		void WriteHostState(const proto::HostState& state);
		bool ReadMcState(proto::McState& out) const;

		std::uint8_t* Base() const { return base_; }
		HANDLE Mapping() const { return mapping_; }

		template <class T>
		T* At(std::uint64_t off) const
		{
			return reinterpret_cast<T*>(base_ + off);
		}

	private:
		HANDLE mapping_{ nullptr };
		void* view_{ nullptr };
		std::uint8_t* base_{ nullptr };
		std::uint32_t hostSeq_{ 0 };
	};
}
