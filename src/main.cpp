#include "Menu.h"
#include "Settings.h"
#include "Wait.h"

namespace
{
	void InitializeLog()
	{
		auto path = logger::log_directory();
		if (!path) {
			return;
		}
		*path /= "DynamicWaitNG.log"sv;
		auto log = std::make_shared<spdlog::logger>("global log"s, std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v"s);
	}

	void OnSave(SKSE::SerializationInterface* a_intfc) { Wait::Save(a_intfc); }
	void OnLoad(SKSE::SerializationInterface* a_intfc) { Wait::Load(a_intfc); }
	void OnRevert(SKSE::SerializationInterface*) { Wait::OnRevert(); }

	void OnMessage(SKSE::MessagingInterface::Message* a_message)
	{
		switch (a_message->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			Wait::Init();
			Menu::Register();
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			Wait::Cancel(false);
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			Wait::OnPostLoadGame();
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	InitializeLog();

	Settings::Get().Load();
	if (Settings::Get().debugLog) {
		spdlog::set_level(spdlog::level::debug);
	}
	// writes the file when there is none yet, so there is something to edit
	Settings::Get().Save();

	const auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID('DWNG');
	serialization->SetSaveCallback(OnSave);
	serialization->SetLoadCallback(OnLoad);
	serialization->SetRevertCallback(OnRevert);

	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

	logger::info("Dynamic Wait NG loaded");
	return true;
}
