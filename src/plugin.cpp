struct QuestRule {
    TESQuest* form;
    uint16_t stage = 0;
    std::string compare = ">=";
};

struct GlobalVarRule {
    TESGlobal* form;
    float value = 1.0f;
    std::string compare = "=";
};

struct Rule {
    std::vector<std::string> keywords;
    BGSPerk* perk;
    std::vector<BGSLocation*> locations;
    std::vector<TESFaction*> vendorFactions;
    GlobalVarRule global{};
    QuestRule quest{};
    BGSPerk* conditionPerk;
    float buyMult = 1.0f;
    float sellMult = 1.0f;
    bool defaultMults = true;
    TESGlobal* buyMultGLOB;
    TESGlobal* sellMultGLOB;
    // bool checkStolen = false;
    // bool stolen = false;
};

std::vector<Rule> rules;
TESGlobal* disabled;
PlayerCharacter* player;

// config
bool showIndicators = true;
bool colorCode = true;
std::string favorablePriceColor = "0x00ff00";
std::string unfavorablePriceColor = "0xff0000";
int indicatorPadding = 5;

static bool PlayerIsInLocation(BGSLocation* location) {
    BGSLocation* current = player->GetCurrentLocation();
    while (current) {
        if (current == location) return true;
        current = current->parentLoc;
    }
    return false;
}

static bool PlayerIsInAnyLocations(const std::vector<BGSLocation*>& locations) {
    for (auto location : locations) {
        if (PlayerIsInLocation(location)) {
            return true;
        }
    }
    return false;
}

static RE::Actor* GetBarterTarget() {
    ObjectRefHandle speaker = MenuTopicManager::GetSingleton()->speaker;
    if (speaker) {
        auto ref = speaker.get();
        if (!ref) return nullptr;
        return ref->As<Actor>();
    }
    return nullptr;
}

static bool IsInAnyFaction(Actor* actor, const std::vector<TESFaction*>& vendorFactions) {
    for (auto faction : vendorFactions) {
        if (actor->IsInFaction(faction)) {
            return true;
        }
    }
    return false;
}

template <typename T>
bool compare(T a, T b, const std::string& op) {
    if (op == "=" || op == "==") return a == b;
    if (op == "<") return a < b;
    if (op == "<=") return a <= b;
    if (op == ">") return a > b;
    if (op == ">=") return a >= b;
    if (op == "!=") return a != b;
    return false;
}

static bool ValidateRule(Rule& theRule) {
    if (theRule.perk) {
        if (!player->HasPerk(theRule.perk)) return false;
    }
    if (theRule.global.form) {
        if (!compare(theRule.global.form->value, theRule.global.value, theRule.global.compare)) return false;
    }
    if (theRule.locations.size()) {
        if (!PlayerIsInAnyLocations(theRule.locations)) return false;
    }
    if (theRule.vendorFactions.size()) {
        Actor* target = GetBarterTarget();
        if (!target) return false;
        if (!IsInAnyFaction(target, theRule.vendorFactions)) return false;
    }
    if (theRule.quest.form) {
        if (!compare(theRule.quest.form->currentStage, theRule.quest.stage, theRule.quest.compare)) return false;
    }
    if (theRule.conditionPerk) {
        if (!theRule.conditionPerk->perkConditions.IsTrue(player, GetBarterTarget())) return false;
    }
    return true;
}

static void Inject(BSFixedString menuName) {
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
    bool bInject = false;
    for (auto& rule : rules) {
        if (!ValidateRule(rule)) continue;
        bInject = true;
        GFxValue keywords;
        movie->CreateArray(&keywords);
        keywords.SetArraySize(rule.keywords.size());
        for (int i = 0; i < rule.keywords.size(); i++) {
            keywords.SetElement(i, GFxValue(rule.keywords[i]));
        }
        GFxValue ruleData;
        movie->CreateObject(&ruleData);
        ruleData.SetMember("keywords", keywords);
        ruleData.SetMember("buy", rule.buyMultGLOB ? rule.buyMultGLOB->value : rule.buyMult);
        ruleData.SetMember("sell", rule.sellMultGLOB ? rule.sellMultGLOB->value : rule.sellMult);
        // if (rule.checkStolen) {
        //     ruleData.SetMember("stolen", rule.stolen);
        // }
        if (rule.defaultMults == false) {
            ruleData.SetMember("defaultMults", false);
        }
        data.PushBack(ruleData);
    }
    if (!bInject) return;
    _root.SetMember("DPF", data);

    // add config
    GFxValue config;
    movie->CreateObject(&config);
    config.SetMember("showIndicators", showIndicators);
    config.SetMember("colorCode", colorCode);
    config.SetMember("favorablePriceColor", GFxValue(favorablePriceColor));
    config.SetMember("unfavorablePriceColor", GFxValue(unfavorablePriceColor));
    config.SetMember("indicatorPadding", indicatorPadding);
    _root.SetMember("DPF_Config", config);

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

static void ParseData(const json& data) {
    rules.reserve(data.size());
    for (const auto& item : data) {
        Rule newRule{};
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
                if (!newRule.perk) continue;
            }
            // if (conditions.contains("stolen")) {
            //     newRule.checkStolen = true;
            //     newRule.stolen = conditions.at("stolen").get<bool>();
            // }
            if (conditions.contains("locations")) {
                const auto& locations = conditions.at("locations");
                if (locations.is_string()) {
                    BGSLocation* form = TESForm::LookupByEditorID<BGSLocation>(locations.get<std::string>());
                    if (form) {
                        newRule.locations = {form};
                    } else {
                        continue;  // don't need to add
                    }
                } else if (locations.is_array()) {
                    for (const auto& location : locations) {
                        BGSLocation* form = TESForm::LookupByEditorID<BGSLocation>(location.get<std::string>());
                        if (form) {
                            newRule.locations.push_back(form);
                        }
                    }
                    if (newRule.locations.empty()) {
                        continue;  // none of the locations were valid, throw it away
                    }
                }
            }
            if (conditions.contains("vendorFactions")) {
                const auto& vendorFactions = conditions.at("vendorFactions");
                if (vendorFactions.is_string()) {
                    TESFaction* form = TESForm::LookupByEditorID<TESFaction>(vendorFactions.get<std::string>());
                    if (form) {
                        newRule.vendorFactions = {form};
                    } else {
                        continue;
                    }
                } else if (vendorFactions.is_array()) {
                    for (const auto& faction : vendorFactions) {
                        TESFaction* form = TESForm::LookupByEditorID<TESFaction>(faction.get<std::string>());
                        if (form) {
                            newRule.vendorFactions.push_back(form);
                        }
                    }
                    if (newRule.vendorFactions.empty()) {
                        continue;  // none of the vendorFactions were valid, throw it away
                    }
                }
            }
            if (conditions.contains("global")) {
                const auto& global = conditions.at("global");
                if (global.contains("name")) {
                    TESGlobal* form = TESForm::LookupByEditorID<TESGlobal>(global.at("name").get<std::string>());
                    if (form) {
                        newRule.global.form = form;
                        if (global.contains("value")) {
                            newRule.global.value = global.at("value").get<float>();
                        }
                        if (global.contains("comparison")) {
                            newRule.global.compare = global.at("comparison").get<std::string>();
                        }
                    } else {
                        continue;
                    }
                }
            }
            if (conditions.contains("quest")) {
                const auto& quest = conditions.at("quest");
                if (quest.contains("id")) {
                    TESQuest* form = TESForm::LookupByEditorID<TESQuest>(quest.at("id").get<std::string>());
                    if (form) {
                        newRule.quest.form = form;
                        if (quest.contains("stage")) {
                            newRule.quest.stage = quest.at("stage").get<uint16_t>();
                        }
                        if (quest.contains("comparison")) {
                            newRule.quest.compare = quest.at("comparison").get<std::string>();
                        }
                    } else {
                        continue;
                    }
                }
            }
            if (conditions.contains("conditionForm")) {
                TESForm* form = TESForm::LookupByEditorID(conditions.at("conditionForm").get<std::string>());
                if (form) {
                    if (form->GetFormType() == FormType::Perk) {
                        newRule.conditionPerk = form->As<BGSPerk>();
                    }
                } else {
                    continue;
                }
            }
        }
        if (item.contains("buyMult")) {
            const auto& value = item.at("buyMult");
            if (value.is_string()) {
                TESGlobal* form = TESForm::LookupByEditorID<TESGlobal>(value.get<std::string>());
                if (!form) continue;
                newRule.buyMultGLOB = form;
            } else if (value.is_number_float()) {
                newRule.buyMult = value.get<float>();
            } else {
                continue;
            }
        }
        if (item.contains("sellMult")) {
            const auto& value = item.at("sellMult");
            if (value.is_string()) {
                TESGlobal* form = TESForm::LookupByEditorID<TESGlobal>(value.get<std::string>());
                if (!form) continue;
                newRule.sellMultGLOB = form;
            } else if (value.is_number_float()) {
                newRule.sellMult = value.get<float>();
            } else {
                continue;
            }
        }
        // weather default price multipliers should be disabled for this item
        if (item.contains("defaultMults") && item.at("defaultMults").get<bool>() == false) {
            newRule.defaultMults = false;
        }
        rules.emplace_back(std::move(newRule));
    }
}

static void BuildRules() {
    const std::filesystem::path dir = "Data/SKSE/Plugins/DynamicPricing";
    if (!std::filesystem::exists(dir)) return;
    for (auto& file : std::filesystem::directory_iterator(dir)) {
        std::ifstream ifile{file.path()};
        if (!ifile) continue;
        try {
            json data = json::parse(ifile);
            if (data.is_discarded()) continue;
            ParseData(data);
        } catch (...) {
        }
    }
}

static bool IsDisabled() { return disabled && disabled->value == 1.0f; }

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

static void LoadConfig() {
    const std::filesystem::path configFile = "Data/SKSE/Plugins/DynamicPricing.json";
    if (!std::filesystem::exists(configFile)) return;
    std::ifstream ifile{configFile};
    if (!ifile) return;
    try {
        json data = json::parse(ifile);
        if (data.is_discarded()) return;
        showIndicators = data.at("showIndicators").get<bool>();
        colorCode = data.at("colorCode").get<bool>();
        favorablePriceColor = data.at("favorablePriceColor").get<std::string>();
        unfavorablePriceColor = data.at("unfavorablePriceColor").get<std::string>();
        indicatorPadding = data.at("indicatorPadding").get<int>();
    } catch (...) {
    }
}

static void OnDataLoad() {
    BuildRules();
    if (!rules.empty()) {
        LoadConfig();
        disabled = TESForm::LookupByEditorID<TESGlobal>("DynamicPricing_Disabled");
        player = PlayerCharacter::GetSingleton();
        static MyEventSink g_EventSink;
        UI::GetSingleton()->AddEventSink(&g_EventSink);
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            OnDataLoad();
        };
    });

    return true;
}