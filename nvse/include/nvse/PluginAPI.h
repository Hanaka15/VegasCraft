#pragma once

// Standalone subset of xNVSE PluginAPI for VegasCraft (Win32 plugin).
// Prefer linking against a full xNVSE checkout when building with script commands;
// messaging + query/load is enough for the VegasCraft bridge.
// Source of truth: https://github.com/xNVSE/NVSE (nvse/nvse/PluginAPI.h)

#include <cstdint>

using UInt32 = std::uint32_t;
using PluginHandle = UInt32;

inline constexpr PluginHandle kPluginHandle_Invalid = 0xFFFFFFFFu;

enum
{
	kInterface_Serialization = 0,
	kInterface_Console,
	kInterface_Messaging,
	kInterface_CommandTable,
	kInterface_StringVar,
	kInterface_ArrayVar,
	kInterface_Script,
	kInterface_Data,
	kInterface_EventManager,
	kInterface_Logging,
	kInterface_PlayerControls,
	kInterface_Max
};

#define MAKE_NEW_VEGAS_VERSION_EX(major, minor, build, sub) \
	(((major & 0xFF) << 24) | ((minor & 0xFF) << 16) | ((build & 0xFFF) << 4) | (sub & 0xF))
#define MAKE_NEW_VEGAS_VERSION(major, minor, build) MAKE_NEW_VEGAS_VERSION_EX(major, minor, build, 0)

#define RUNTIME_VERSION_1_4_0_525 MAKE_NEW_VEGAS_VERSION(4, 0, 525)
#define PACKED_NVSE_VERSION MAKE_NEW_VEGAS_VERSION(6, 4, 9)

struct PluginInfo
{
	enum
	{
		kInfoVersion = 1
	};
	UInt32 infoVersion;
	const char* name;
	UInt32 version;
};

struct CommandInfo;
enum CommandReturnType : UInt32
{
	kRetnType_Default = 0
};

struct NVSEInterface
{
	UInt32 nvseVersion;
	UInt32 runtimeVersion;
	UInt32 editorVersion;
	UInt32 isEditor;
	bool (*RegisterCommand)(CommandInfo* info);
	void (*SetOpcodeBase)(UInt32 opcode);
	void* (*QueryInterface)(UInt32 id);
	PluginHandle (*GetPluginHandle)(void);
	bool (*RegisterTypedCommand)(CommandInfo* info, CommandReturnType retnType);
	const char* (*GetRuntimeDirectory)();
	UInt32 isNogore;
	void (*InitExpressionEvaluatorUtils)(void* utils);
	bool (*RegisterTypedCommandVersion)(CommandInfo* info, CommandReturnType retnType, UInt32 requiredPluginVersion);
};

struct NVSEMessagingInterface
{
	enum
	{
		kVersion = 4
	};

	enum
	{
		kMessage_PostLoad = 0,
		kMessage_ExitGame,
		kMessage_ExitToMainMenu,
		kMessage_LoadGame,
		kMessage_SaveGame,
		kMessage_ScriptPrecompile,
		kMessage_PreLoadGame,
		kMessage_ExitGame_Console,
		kMessage_PostLoadGame,
		kMessage_PostPostLoad,
		kMessage_RuntimeScriptError,
		kMessage_DeleteGame,
		kMessage_RenameGame,
		kMessage_RenameNewGame,
		kMessage_NewGame,
		kMessage_DeleteGameName,
		kMessage_RenameGameName,
		kMessage_RenameNewGameName,
		kMessage_DeferredInit,
		kMessage_ClearScriptDataCache,
		kMessage_MainGameLoop,
		kMessage_ScriptCompile,
		kMessage_EventListDestroyed,
		kMessage_PostQueryPlugins,
		kMessage_OnFramePresent, // about to present a frame; data = int* isLoadingScreen (not D3D device)
		kMessage_ReloadConfig,
	};

	struct Message
	{
		const char* sender;
		UInt32 type;
		UInt32 dataLen;
		void* data;
	};

	using EventCallback = void (*)(Message* msg);

	UInt32 version;
	void (*RegisterListener)(PluginHandle listener, const char* sender, EventCallback handler);
	void (*Dispatch)(PluginHandle from, UInt32 msgType, void* data, UInt32 dataLen, const char* receiver);
};

struct NVSEConsoleInterface
{
	enum
	{
		kVersion = 3
	};
	UInt32 version;
	bool (*RunScriptLine)(const char* buf, void* object);
	bool (*RunScriptLine2)(const char* buf, void* callingRefr, bool bSuppressConsoleOutput);
};
