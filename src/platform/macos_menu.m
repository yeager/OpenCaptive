#import <Cocoa/Cocoa.h>
#include "macos_menu.h"

void macos_localize_menus(void) {
    NSString *lang = [[NSBundle mainBundle] preferredLocalizations].firstObject;
    if (!lang) return;

    NSMenu *mainMenu = [NSApp mainMenu];
    if (!mainMenu) return;

    NSDictionary *translations = @{
        @"sv": @{
            @"Window": @"Fönster",
            @"Minimize": @"Minimera",
            @"Zoom": @"Zooma",
            @"Bring All to Front": @"Lägg alla längst fram",
            @"Hide": @"Göm",
            @"Hide Others": @"Göm andra",
            @"Show All": @"Visa alla",
            @"Quit": @"Avsluta",
            @"About": @"Om",
        },
        @"da": @{
            @"Window": @"Vindue",
            @"Minimize": @"Minimer",
            @"Zoom": @"Zoom",
            @"Bring All to Front": @"Anbring alle forrest",
            @"Hide": @"Skjul",
            @"Hide Others": @"Skjul andre",
            @"Show All": @"Vis alle",
            @"Quit": @"Slut",
            @"About": @"Om",
        },
        @"de": @{
            @"Window": @"Fenster",
            @"Minimize": @"Minimieren",
            @"Zoom": @"Zoomen",
            @"Bring All to Front": @"Alle nach vorne bringen",
            @"Hide": @"Ausblenden",
            @"Hide Others": @"Andere ausblenden",
            @"Show All": @"Alle einblenden",
            @"Quit": @"Beenden",
            @"About": @"Über",
        },
        @"es": @{
            @"Window": @"Ventana",
            @"Minimize": @"Minimizar",
            @"Zoom": @"Zoom",
            @"Bring All to Front": @"Traer todo al frente",
            @"Hide": @"Ocultar",
            @"Hide Others": @"Ocultar otros",
            @"Show All": @"Mostrar todo",
            @"Quit": @"Salir de",
            @"About": @"Acerca de",
        },
        @"fi": @{
            @"Window": @"Ikkuna",
            @"Minimize": @"Pienennä",
            @"Zoom": @"Zoomaa",
            @"Bring All to Front": @"Tuo kaikki eteen",
            @"Hide": @"Kätke",
            @"Hide Others": @"Kätke muut",
            @"Show All": @"Näytä kaikki",
            @"Quit": @"Lopeta",
            @"About": @"Tietoja:",
        },
        @"fr": @{
            @"Window": @"Fenêtre",
            @"Minimize": @"Réduire",
            @"Zoom": @"Zoom",
            @"Bring All to Front": @"Tout ramener au premier plan",
            @"Hide": @"Masquer",
            @"Hide Others": @"Masquer les autres",
            @"Show All": @"Tout afficher",
            @"Quit": @"Quitter",
            @"About": @"À propos de",
        },
        @"it": @{
            @"Window": @"Finestra",
            @"Minimize": @"Riduci a icona",
            @"Zoom": @"Ingrandisci",
            @"Bring All to Front": @"Porta tutto in primo piano",
            @"Hide": @"Nascondi",
            @"Hide Others": @"Nascondi altri",
            @"Show All": @"Mostra tutti",
            @"Quit": @"Esci da",
            @"About": @"Informazioni su",
        },
        @"ja": @{
            @"Window": @"ウインドウ",
            @"Minimize": @"しまう",
            @"Zoom": @"拡大/縮小",
            @"Bring All to Front": @"すべてを手前に移動",
            @"Hide": @"隠す",
            @"Hide Others": @"ほかを隠す",
            @"Show All": @"すべてを表示",
            @"Quit": @"終了",
            @"About": @"について",
        },
        @"ko": @{
            @"Window": @"윈도우",
            @"Minimize": @"최소화",
            @"Zoom": @"확대/축소",
            @"Bring All to Front": @"모두 앞으로 가져오기",
            @"Hide": @"가리기",
            @"Hide Others": @"기타 가리기",
            @"Show All": @"모두 보기",
            @"Quit": @"종료",
            @"About": @"에 관하여",
        },
        @"nl": @{
            @"Window": @"Venster",
            @"Minimize": @"Minimaliseer",
            @"Zoom": @"Zoom",
            @"Bring All to Front": @"Breng alles naar voren",
            @"Hide": @"Verberg",
            @"Hide Others": @"Verberg andere",
            @"Show All": @"Toon alles",
            @"Quit": @"Stop",
            @"About": @"Over",
        },
        @"no": @{
            @"Window": @"Vindu",
            @"Minimize": @"Minimer",
            @"Zoom": @"Zoom",
            @"Bring All to Front": @"Flytt alle til fronten",
            @"Hide": @"Skjul",
            @"Hide Others": @"Skjul andre",
            @"Show All": @"Vis alle",
            @"Quit": @"Avslutt",
            @"About": @"Om",
        },
        @"pl": @{
            @"Window": @"Okno",
            @"Minimize": @"Minimalizuj",
            @"Zoom": @"Powiększenie",
            @"Bring All to Front": @"Przesuń wszystko na wierzch",
            @"Hide": @"Ukryj",
            @"Hide Others": @"Ukryj pozostałe",
            @"Show All": @"Pokaż wszystko",
            @"Quit": @"Zakończ",
            @"About": @"O programie",
        },
        @"pt": @{
            @"Window": @"Janela",
            @"Minimize": @"Minimizar",
            @"Zoom": @"Zoom",
            @"Bring All to Front": @"Trazer tudo para a frente",
            @"Hide": @"Ocultar",
            @"Hide Others": @"Ocultar outros",
            @"Show All": @"Mostrar tudo",
            @"Quit": @"Encerrar",
            @"About": @"Sobre o",
        },
        @"ru": @{
            @"Window": @"Окно",
            @"Minimize": @"Убрать в Dock",
            @"Zoom": @"Изменить масштаб",
            @"Bring All to Front": @"Все окна — на передний план",
            @"Hide": @"Скрыть",
            @"Hide Others": @"Скрыть остальные",
            @"Show All": @"Показать все",
            @"Quit": @"Завершить",
            @"About": @"О программе",
        },
        @"zh": @{
            @"Window": @"窗口",
            @"Minimize": @"最小化",
            @"Zoom": @"缩放",
            @"Bring All to Front": @"前置全部窗口",
            @"Hide": @"隐藏",
            @"Hide Others": @"隐藏其他",
            @"Show All": @"全部显示",
            @"Quit": @"退出",
            @"About": @"关于",
        },
        @"cs": @{
            @"Window": @"Okno",
            @"Minimize": @"Minimalizovat",
            @"Zoom": @"Zvětšení",
            @"Bring All to Front": @"Přenést vše do popředí",
            @"Hide": @"Skrýt",
            @"Hide Others": @"Skrýt ostatní",
            @"Show All": @"Zobrazit vše",
            @"Quit": @"Ukončit",
            @"About": @"O aplikaci",
        },
        @"hu": @{
            @"Window": @"Ablak",
            @"Minimize": @"Kicsinyítés",
            @"Zoom": @"Nagyítás",
            @"Bring All to Front": @"Összes előtérbe hozása",
            @"Hide": @"Elrejtés",
            @"Hide Others": @"Többi elrejtése",
            @"Show All": @"Összes megjelenítése",
            @"Quit": @"Kilépés",
            @"About": @"Névjegy:",
        },
        @"ro": @{
            @"Window": @"Fereastră",
            @"Minimize": @"Minimizare",
            @"Zoom": @"Zoom",
            @"Bring All to Front": @"Adu-le pe toate în față",
            @"Hide": @"Ascunde",
            @"Hide Others": @"Ascunde celelalte",
            @"Show All": @"Afișează tot",
            @"Quit": @"Ieșire din",
            @"About": @"Despre",
        },
    };

    NSDictionary *tr = translations[lang];
    if (!tr) return;

    NSString *appName = @"OpenCaptive";

    for (NSMenuItem *topItem in mainMenu.itemArray) {
        NSMenu *sub = topItem.submenu;
        if (!sub) continue;

        if ([sub.title isEqualToString:@"Window"]) {
            NSString *winTitle = tr[@"Window"];
            if (winTitle) {
                sub.title = winTitle;
                topItem.title = winTitle;
            }
        }

        for (NSMenuItem *item in sub.itemArray) {
            NSString *title = item.title;
            if ([title hasPrefix:@"About "]) {
                item.title = [NSString stringWithFormat:@"%@ %@",
                              tr[@"About"] ?: @"About", appName];
            } else if ([title hasPrefix:@"Hide "] &&
                       ![title isEqualToString:@"Hide Others"]) {
                item.title = [NSString stringWithFormat:@"%@ %@",
                              tr[@"Hide"] ?: @"Hide", appName];
            } else if ([title isEqualToString:@"Hide Others"]) {
                item.title = tr[@"Hide Others"] ?: title;
            } else if ([title isEqualToString:@"Show All"]) {
                item.title = tr[@"Show All"] ?: title;
            } else if ([title hasPrefix:@"Quit "]) {
                item.title = [NSString stringWithFormat:@"%@ %@",
                              tr[@"Quit"] ?: @"Quit", appName];
            } else if ([title isEqualToString:@"Minimize"]) {
                item.title = tr[@"Minimize"] ?: title;
            } else if ([title isEqualToString:@"Zoom"]) {
                item.title = tr[@"Zoom"] ?: title;
            } else if ([title isEqualToString:@"Bring All to Front"]) {
                item.title = tr[@"Bring All to Front"] ?: title;
            }
        }
    }
}
