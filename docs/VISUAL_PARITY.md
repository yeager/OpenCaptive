# Visuell paritet

> Uppdaterad för v1.1.102. Dokumentet skiljer mellan reproducerbara originalbilder och prototypgrafik.

Paritetskontroller får bara använda avkodade originalresurser eller fångster
från en sådan avkodningsväg. Den tidigare CI-snapshoten av en hembyggd
Captive-korridor har tagits bort: en stabil hash av syntetiska pixlar är inte
en paritetskontroll. Eftersom originalmedia inte ingår i källträdet körs den
fulla visuella kontrollen lokalt mot användarens verifierade media.

Liberation har dessutom två direkt verifierade originalregioner. Första
planarbilden ur den hashidentifierade CD32-resursen avkodas separat och
jämförs sedan pixel för pixel med rätt rektangel i OpenCaptives native-fångst:

| Presentation | FORM/ANIM SHA-256 | Region | Avvikande pixlar |
| --- | --- | --- | --- |
| Intro | `e7e35f1b491fafd95da260abcb1c1c402140601840c5f98ae7282069fd30b269` | 320×162 vid `(0,47)` | 0 av 51 840 |
| Stad | `b94a450c12428af9a22b8bb8c31fca74cdc2b2bd3be3dc9c7a1eadd7e6576101` | 320×167 vid `(0,44)` | 0 av 53 440 |

De direkt avkodade PPM-bilderna har SHA-256
`c65df735ccd785dee5cbe118c3f51153f270f6260411d3e04c6ca278b2d6fab3`
respektive
`b7c326d1cdd36bb3574b33add3d68cff9739e7a5e339d800f44af3c79f510bb1`.
Detta är bevis för just de första presentationrutorna, inte för hela
Liberation-spelvyn, dess animerade lager eller spelstate.

Den ursprungliga missionsmenyn avkodas också direkt från den hashverifierade
AMOS-banken `d6bb0dd9c578beb8e84ddf9f458f0be43ec158b2b261491d023e972d2812c2d2`.
Bankens enda sprite är en femplanskomposition på 320×109 pixlar. OpenCaptive
visar den mellan introt och den nuvarande stadsreferensen, och originalets
`Game on...`-rektangel är klickbar. Detta verifierar menybilden, men inte
originalspelets senare stadssimulering eller UI-state.

Kör lokalt med:

```sh
./build/opencaptive --verify-data all --data /path/to/media
```

För Liberation omfattar verifieringen även PPM-hashen för de två avkodade
första bildrutorna ovan. Den bekräftar alltså att originalets pixlar avkodas
oförändrat; den säger fortfarande inget om senare animation, spelstate eller
interaktion.

Vid en avsiktlig visuell ändring ska den nya bilden granskas först. Uppdatera
sedan endast den berörda referenshashen och dokumentera den synliga ändringen i
samma ändring.

## Reproducerbara spelbilder

`--capture-frame <ppm>` startar valt spel, skriver den första kompletta interna
bildbufferten i spelens originalupplösning och avslutar sedan. Fångsten sker
före fönsterskalning, systemets färghantering och externa överlägg:

För Captive börjar även headless-fångsten i den verifierade holomap/navigation-
vyn, samma state som normal interaktiv uppstart. Den hoppar alltså inte direkt
till en landad dungeon-checkpoint och använder inte en genererad mission för
att fylla viewporten.

```sh
./build/opencaptive --game captive --data /path/to/media \
  --capture-frame /tmp/captive.ppm
./build/opencaptive --game liberation --data /path/to/media \
  --capture-frame /tmp/liberation.ppm
./build/opencaptive --game liberation --skip-intro --data /path/to/media \
  --capture-frame /tmp/liberation-city.ppm
```

Captive-fångsten kan jämföras med den hashidentifierade HUD-resursen utanför
spelvyn, men detta är inte en fullständig live-spelbildjämförelse. En riktig
spelframe ändrar även delar av monitorerna, kontrollerna och statusytan.
Den fullständiga DOS-panelkompositorn och den live cellbaserade
dungeonrenderingen är ännu inte återställda; den verifierade landningsbilden
är därför ett explicit autentiserat checkpoint, inte ett påstående om
fullständig pixelparitet.

En DOSBox-dump från ett ägt original kan göras till en reproducerbar referens
utan att spelmedia läggs i källträdet. Verktyget accepterar exakt den 1 MiB
stora utdata som skapas av `MEMDUMPBIN 0 0 100000`, skriver dumpens SHA-256 och
extraherar VGA-bufferområdet `0xA0000..0xAFA00` som en 320×200 PPM-bild:

```sh
./build/opencaptive --extract-dos-vga /path/to/MEMDUMP.BIN /tmp/captive-original.ppm
./build/opencaptive --compare-frames /tmp/captive-original.ppm /tmp/captive.ppm
```

`captive_descriptor_match` är en snävare analysgrind för den dokumenterade
DOS-renderaren. Den godtar endast den kända, hashverifierade 1 MiB-dumpen och
en bild som är exakt extraherad ur just den dumpen. Den identifierar därmed
inte speldata med filnamn och kan inte av misstag jämföra mot en bild från ett
annat ögonblick i spelet:

```sh
./build/opencaptive --extract-dos-vga /path/to/MEMDUMP.BIN /tmp/captive-original.ppm
./build/captive_descriptor_match /path/to/MEMDUMP.BIN /tmp/captive-original.ppm
```

Kontrollen bekräftar hittills två statiska paneler i Captive-vyn. Den bevisar
inte att den dynamiska dungeonrenderingen är återställd.

För delområden med egen paritetsgrind används samma exakta jämförelse med en
rektangel. Den returnerar noll endast om samtliga pixlar inom rektangeln är
identiska och två för en ogiltig rektangel:

```sh
./build/opencaptive --compare-frames-rect \
  /tmp/captive-original.ppm /tmp/captive.ppm 32 55 144 112
```

När originalreferensen är en fristående FORM-bild medan spelfångsten har en
större intern bildyta används olika startkoordinater utan att skapa en
paddingbild. Stadsformen ovan kontrolleras exempelvis så här:

```sh
./build/opencaptive --compare-frames-regions \
  /tmp/liberation-city-form.ppm /tmp/liberation-city.ppm \
  0 0 0 44 320 167
```

Kommandot kräver exakt lika stora regioner och returnerar noll endast när
samtliga RGB-pixlar är identiska.

Det gör att HUD, viewport och senare Liberation-lager kan mätas var för sig
utan att ett känt oåterställt område döljer en förbättring i ett annat.

Den kända referensen med SHA-256
`9003c4a8818cb97f8299ac90cfe51e90e535ab9a725545526fe75f14ddb8dd7e` visar en
genuin Captive-utomhusvy. OpenCaptives nuvarande Captive-fångst skiljer sig i
10 558 av 16 128 pixlar i viewporten och i 8 376 av 47 872 pixlar utanför den.
Siffrorna är ett aktivt felmått, inte ett godkänt paritetsresultat.
`--compare-frames` returnerar noll endast när alla pixlar är identiska och ett
när någon pixel avviker, vilket gör den lämplig som en lokal eller framtida
CI-kontroll efter att originalbilderna har granskats.

`captive_panel_match` använder samtliga hashidentifierade PL5-källytor för
Captive, inklusive väggar, exteriör- och interiörvarianter, animationer,
dörrar, tak, objekt och UI-arbetsytor, samt en sådan 320×200-referensbild.
Den söker utan filnamn efter exakta 8×8-regioner från viewporten
`(32,55,144,112)` och skriver både mål- och källkoordinat med respektive
SHA-256. Mot referensen
`9003c4a8818cb97f8299ac90cfe51e90e535ab9a725545526fe75f14ddb8dd7e`
återfinns 108 av 252 rutor direkt. Resterande rutor är överlappade eller
kompositerade, vilket är förväntat för originalets panelblittrar och visar
var den fortsatta rendereråtervinningen måste ske.

```sh
./build/captive_panel_match /path/to/media /tmp/captive-original.ppm
```

## Captive-panelblad

Fed7-E från den verifierade Amiga-ADF:en avkodas via RNC1-old till fem
bitplan och omvandlas till 64 000 palettindex. Den indexbufferten har
SHA-256 `d2efa8a9cbbbaa45e49c82465765836ba173676645e810fda5cd23ef85bd3431`
och är exakt identisk med det motsvarande hashidentifierade DOS-panelbladet:
64 000 av 64 000 pixlar stämmer. Det bevisar att panelkällorna är originaldata
på båda plattformarna. Det bevisar inte ännu den dynamiska cell- och
panelkompositionen i spelvyn.
