# Visuell paritet

Paritetskontroller får bara använda avkodade originalresurser eller fångster
från en sådan avkodningsväg. Den tidigare CI-snapshoten av en hembyggd
Captive-korridor har tagits bort: en stabil hash av syntetiska pixlar är inte
en paritetskontroll. Eftersom originalmedia inte ingår i källträdet körs den
fulla visuella kontrollen lokalt mot användarens verifierade media.

Liberation har dessutom en direkt verifierad originalregion: den första
320×167-rutan ur den hashidentifierade CD32 `FORM/ANIM`-resursen avkodas till
planar bild och jämförs med OpenCaptives fångade raster. Båda RGB-buffertarna
har SHA-256 `c546bebd107928a5721cd1a33d7e458098b134d4cd86c48b6547f8b615abbdae`.
Detta är bevis för den rutan, inte för hela Liberation-spelvyn eller dess
animerade lager.

Kör lokalt med:

```sh
./build/opencaptive --verify-data all --data /path/to/media
```

Vid en avsiktlig visuell ändring ska den nya bilden granskas först. Uppdatera
sedan endast den berörda referenshashen och dokumentera den synliga ändringen i
samma ändring.

## Reproducerbara spelbilder

`--capture-frame <ppm>` startar valt spel, skriver den första kompletta interna
bildbufferten i spelens originalupplösning och avslutar sedan. Fångsten sker
före fönsterskalning, systemets färghantering och externa överlägg:

```sh
./build/opencaptive --game captive --data /path/to/media \
  --capture-frame /tmp/captive.ppm
./build/opencaptive --game liberation --data /path/to/media \
  --capture-frame /tmp/liberation.ppm
./build/opencaptive --game liberation --skip-intro --data /path/to/media \
  --capture-frame /tmp/liberation-city.ppm
```

Captive-fångsten kan jämföras med den hashidentifierade HUD-resursen utanför
spelvyn, men detta är inte en fullständig spelbildsjämförelse. En riktig
spelframe ändrar även delar av monitorerna, kontrollerna och statusytan.
I standardläget lämnas den dynamiska rektangeln `(32,55,144,112)` orörd tills
originalrenderingen är återställd; den fylls alltså inte med syntetisk grafik.

En DOSBox-dump från ett ägt original kan göras till en reproducerbar referens
utan att spelmedia läggs i källträdet. Verktyget accepterar exakt den 1 MiB
stora utdata som skapas av `MEMDUMPBIN 0 0 100000`, skriver dumpens SHA-256 och
extraherar VGA-bufferområdet `0xA0000..0xAFA00` som en 320×200 PPM-bild:

```sh
./build/opencaptive --extract-dos-vga /path/to/MEMDUMP.BIN /tmp/captive-original.ppm
./build/opencaptive --compare-frames /tmp/captive-original.ppm /tmp/captive.ppm
```

För delområden med egen paritetsgrind används samma exakta jämförelse med en
rektangel. Den returnerar noll endast om samtliga pixlar inom rektangeln är
identiska och två för en ogiltig rektangel:

```sh
./build/opencaptive --compare-frames-rect \
  /tmp/captive-original.ppm /tmp/captive.ppm 32 55 144 112
```

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

## Captive-panelblad

Fed7-E från den verifierade Amiga-ADF:en avkodas via RNC1-old till fem
bitplan och omvandlas till 64 000 palettindex. Den indexbufferten har
SHA-256 `d2efa8a9cbbbaa45e49c82465765836ba173676645e810fda5cd23ef85bd3431`
och är exakt identisk med det motsvarande hashidentifierade DOS-panelbladet:
64 000 av 64 000 pixlar stämmer. Det bevisar att panelkällorna är originaldata
på båda plattformarna. Det bevisar inte ännu den dynamiska cell- och
panelkompositionen i spelvyn.
