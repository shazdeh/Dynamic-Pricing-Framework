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

class MyEventSink : public RE::BSTEventSink<MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
        if (event->menuName == BarterMenu::MENU_NAME && event->opening) {
            Inject(event->menuName);
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

void OnDataLoad() { UI::GetSingleton()->AddEventSink(new MyEventSink()); }

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            OnDataLoad();
        };
    });

    return true;
}