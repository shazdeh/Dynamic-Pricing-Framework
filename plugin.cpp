struct Rule {
    std::vector<std::string> keywords;
    BGSPerk* perk;
    std::vector<BGSLocation*> locations;
    TESGlobal* global;
    float gloablVal = 1.0f;
    float buyMult = 1.0f;
    float sellMult = 1.0f;
};

std::vector<Rule> rules;
TESGlobal* disabled;
Actor* player;

bool ValidateRule(Rule& theRule) {
    if (theRule.perk) {
        if (!player->HasPerk(theRule.perk)) return false;
    }
    if (theRule.global) {
        if (theRule.global->value != theRule.gloablVal) return false;
    }
    return true;
}

void Inject(BSFixedString menuName) {
    auto ui = UI::GetSingleton();
    if (!ui) return;

    GPtr<IMenu> menu = ui->GetMenu(menuName);
    if (!menu || !menu->uiMovie) {
        return;
    }

    auto movie = menu->uiMovie;

    GFxValue _root;
    movie->GetVariable(&_root, "_root");

    GFxValue data;
    movie->CreateArray(&data);
    for (auto& rule : rules) {
        if (ValidateRule(rule)) {
            
        }
    }
    _root.SetMember("DPF", data);

    GFxValue args[2];
    args[0] = GFxValue("DPF");
    args[1] = GFxValue(953);
    _root.Invoke("createEmptyMovieClip", nullptr, args, 2);
    if (movie->GetVariable(&_root, "_root.DPF")) {
        GFxValue args[1];
        args[0] = GFxValue("dynamicpricing_inject.swf");
        _root.Invoke("loadMovie", nullptr, args, 1);
    }
}

void ParseData(const json& data) {
    rules.reserve(data.size());
    for (const auto& item : data) {
        Rule newRule;
        if (item.contains("itemKeyword")) {
            const auto& kw = item.at("itemKeyword");
            if (kw.is_string()) {
                newRule.keywords.push_back(kw.get<std::string>());
            } else if (kw.is_array()) {
                newRule.keywords = kw.get<std::vector<std::string>>();
            }
        }
        if (item.contains("conditions")) {
            auto& conditions = item.at("conditions");
            if (conditions.contains("perk")) {
                newRule.perk = TESForm::LookupByEditorID<BGSPerk>(conditions.at("perk").get<std::string>());
            }
            if (conditions.contains("location")) {
            }
        }
        if (item.contains("changes")) {
            auto& changes = item.at("changes");
            if (changes.contains("buy")) {
                newRule.buyMult = changes.at("buy").get<float>();
            }
            if (changes.contains("sell")) {
                newRule.sellMult = changes.at("sell").get<float>();
            }
        }
        rules.emplace_back(std::move(newRule));
    }
}

void BuildRules() {
    const std::filesystem::path dir = "Data/SKSE/Plugins/DynamicPricing";
    if (!std::filesystem::exists(dir)) {
        return;
    }
    for (auto& file : std::filesystem::directory_iterator(dir)) {
        std::ifstream ifile{file.path()};
        if (!ifile) continue;
        json data = json::parse(ifile, nullptr, false);
        if (data.is_discarded()) continue;
        ParseData(data);
    }
}

bool PlayerIsInLocation(BGSLocation* location) { return true; }

bool IsDisabled() { return disabled && disabled->value == 1.0f; }

class MyEventSink : public RE::BSTEventSink<MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
        if (event->menuName == BarterMenu::MENU_NAME && event->opening) {
            if (!IsDisabled()) {
                Inject(event->menuName);
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

void OnDataLoad() {
    BuildRules();
    if (!rules.empty()) {
        disabled = TESForm::LookupByEditorID<TESGlobal>("DynamicPricing_Disabled");
        player = PlayerCharacter::GetSingleton();
        UI::GetSingleton()->AddEventSink(new MyEventSink());
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            OnDataLoad();
        };
    });

    return true;
}