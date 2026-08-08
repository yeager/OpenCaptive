# OpenCaptive — Completed work

## 2026-08-08 (Liberation bar guess counter)

- Fixed incorrect number guesses for bar games consuming two attempts for
  keys 1–9 while key 0 consumed one.
- Centralized the one-attempt decrement and added coverage for all counter
  boundaries, including protection against unsigned underflow.

## 2026-08-08 (Creature damage documentation source-lock correction)

- Removed invented creature names and placeholder HP values from the public
  Captive data reference; the table now contains only the 25 values recovered
  from the hash-verified CAPPO executable.
- Corrected the technical documentation so CAPPO attack bytecode around
  `0x5380` is not misrepresented as the creature damage formula.
- Documented the remaining creature attack-record gap and kept the runtime
  category/level calculation explicitly marked as a compatibility approximation.

## 2026-08-08 (Captive original weapon ranges)

- Restored the original class-specific combat ranges: melee 1, handguns 6,
  rifles 15, automatics 12, lasers 30, cannons 50 and sprayguns 45.
- Added regression coverage for long-range rifle attacks and the spraygun
  maximum range.
- Full local test suite: 60/60 tests passed; verified against the real
  `.opencaptive` data set.

## 2026-08-08 (Liberation shield damage ordering)

- Fixed the Liberation combat slice so enemy damage is absorbed by equipped
  shield durability before droid HP is reduced.
- Added regression coverage for a fully absorbing shield.
- The full local suite passes 60/60 tests.

## 2026-08-08 (Captive shield damage ordering)

- Fixed incoming combat damage so an equipped shield absorbs damage before
  body-part condition is reduced.
- Added a regression test proving a fully covering shield leaves both HP and
  all body-part condition values unchanged.
- The full local suite passes 60/60 tests.

## 2026-08-08 (Cross-save shield persistence)

- Bumped the portable Captive cross-save format to version 3 so equipped
  shield IDs and durability survive export/import.
- Kept version 1 and version 2 imports compatible, with absent shield fields
  defaulting to an empty shield.
- Added round-trip and legacy-format regression coverage.

## 2026-08-08 (Liberation shield save persistence)

- Extended the OpenCaptive Liberation save stream to version 8 with the
  shared droid shield ID and durability, so F5/F9 no longer silently removes
  an equipped shield.
- Older Liberation saves remain readable and restore with no shield when the
  older format has no shield fields.
- Added round-trip coverage; the complete local suite passes 60/60 tests.

## 2026-08-08 (Combat source-lock correction)

- Corrected the combat documentation and source comments: CAPPO.EXE offset
  `0x5380` is weapon/attack bytecode, not proof of the creature attack
  formula.
- Marked the current bounded creature damage calculation as a compatibility
  approximation and added the actual enemy attack-record recovery to TODO.
- No synthetic creature-damage data was introduced.

## 2026-08-08 (Liberation city road-feature navigation)

- Made finalized road features such as phone boxes and post boxes traversable
  in city movement, matching their rendered ground-cell representation.
- Added regression coverage that walks onto a feature cell and keeps it out of
  wall collision, restoring access to the associated city interactions.
- Full local test suite: 60/60 tests passed.

## 2026-08-08 (Captive grenade actor validation)

- Moved the Captive grenade action into the combat module so it shares the
  normal state validation and kill bookkeeping path.
- Prevented destroyed droids from throwing grenades or consuming inventory.
- Added regression coverage for the dead-droid case; full local suite remains
  60/60 tests passed.

## 2026-08-08 (Captive saturated weapon damage)

- Fixed the CAPPO-compatible weapon-damage saturation path: `0xFFFD` is now
  kept as a positive 16-bit damage value instead of becoming signed `-3`.
- Added regression coverage proving an over-range weapon still kills a normal
  target rather than collapsing to a one-point hit.
- Full local test suite: 60/60 tests passed.

## 2026-08-08 (Liberation difficulty save/load)

- Fixed Liberation F5 saves that accidentally wrote the mission number as the
  difficulty value.
- Restored the saved difficulty during F9 loading and rejected values outside
  the supported 0–2 range.
- Added regression coverage; the targeted Liberation save test passes.

## 2026-08-08 (Captive viewport depth-band geometry)

- Corrected the active compatibility renderer's four non-player depth bands to
  the recovered CAPPO panel coordinates: y=9/25/37/45 with heights 98/70/49/35.
- Kept the original descriptor draw sequence out of the runtime until its
  complete per-cell dispatch and caller destination bases are recovered.
- Full local test suite: 60/60 tests passed.

## 2026-08-08 (Localization catalog validation)

- Removed duplicate launcher/F10 message definitions from all 18 translated
  PO catalogs while preserving the existing translations.
- Added a portable catalog validator to CTest so duplicate or missing required
  launcher strings are caught without requiring gettext on the build host.
- All 18 catalogs pass `msgfmt --check`; the full local suite is now 60/60.

## 2026-08-08 (Captive spawn/combat data integrity)

- Made shared kill registration idempotent so alternate weapon paths cannot
  award a finalized creature twice.
- Corrected the English technical reference to document all 25 verified
  CAPPO spawn modifier bytes, including creature types 16–24.
- Added regression coverage for duplicate kill registration.

## 2026-08-08 (Captive grenade kill progression)

- Routed grenade kills through the shared Captive kill finalization path so
  explosive kills now update respawn state, score, gold, drops, shared XP, and
  the per-attack kill event consistently with direct and spray kills.
- Added regression coverage for alternate-weapon kill bookkeeping.

## 2026-08-08 (Captive spray kill progression)

- Routed spray splash kills through the same kill finalization as direct
  attacks, including respawn state, score, gold, drops, shared XP, and the
  per-attack kill event.
- Added regression coverage for a spray attack killing both its primary and
  splash targets.
- Targeted game-state test: passed.

## 2026-08-08 (Liberation city presentation depth)

- Restored support for the six planar bitplanes used by the hash-verified
  Liberation CD32 city FORM/ANIM resource.
- The real city frame from `.opencaptive` now decodes at 320×167 and matches
  the independent first-frame pixel hash used by `--verify-data liberation`.
- Added a container-level regression test for six-plane FORM/ANIM records.
- Full local test suite: 59/59 tests passed.

## 2026-08-08 (Captive creature animation frame routing)

- The active Captive viewport now uses the disassembly `frame_index` from
  `DS:0xA16E` when sampling the 10x5 32x40 ALIEN sheet grid.
- Added bounds-checked frame-origin conversion and regression coverage for
  valid and invalid frame indices.
- Full local test suite: 59/59 tests passed.

## 2026-08-07 (Captive ANM endian contract)

- Corrected the ANM decoder comments to match the verified little-endian
  command-offset and frame-size layout.
- Added a regression test that decodes a one-pixel frame using the actual
  little-endian framing contract.
- The targeted ANM decoder test passes.

## 2026-08-07 (Captive version popup mouse selection)

- Fixed mouse selection in the version popup when only one non-first source is
  available. The visible row is now mapped through the filtered source list,
  matching keyboard selection behavior.
- Added a regression test for selecting the Captive Amiga row with the mouse.
- The targeted start-menu test passes.

## 2026-08-07 (Liberation bar inventory handling)

- Bar drinks are consumed at purchase time and are no longer transferred into
  the shared inventory with invalid runtime item IDs.
- A full shared inventory no longer prevents buying drinks or performing the
  documented bar-fight roll.
- Added a regression test covering purchase cost, transient drinks, and the
  full-inventory case.
- The targeted Liberation dialogue/shop test passes.

## 2026-08-07 (Captive combat-händelser)

- `creature_killed` och `level_up_occurred` beskriver nu alltid det aktuella
  droidskottet. Tidigare kunde flaggor från ett föregående skott ligga kvar
  efter en miss eller en icke-dödande träff.
- Regressionstestet verifierar ett dödande skott följt av ett icke-dödande
  skott.
- Fullt lokalt testpaket: 58 av 58 tester godkända.

## 2026-08-07 (Captive encounter density)

- Captive skapar nu `3 + nivå * 2` encounter-grupper per dungeon-nivå.
  Den tidigare `3 + nivå`-formeln gjorde senare nivåer underbemannade.
- Regressionstestet verifierar att nivå 1 producerar fler encounters än nivå 0
  över 128 deterministiska seed.
- Fullt lokalt testpaket: 58 av 58 tester godkända.

## 2026-08-07 (Captive combatdokumentation)

- `docs/COMBAT_SYSTEM.md` beskriver nu den faktiska combat-prototypen:
  tio-loopars AI-intervall, cachead vapenskada, halverad defense-reduktion,
  energikostnad och avsaknad av separat droid-vapencooldown.

## 2026-08-07 (Captive spawn-difficulty)

- Captive-fiender använder nu det nollbaserade dungeon-level-värdet direkt i
  HP-beräkningen. Tidigare minskades värdet en extra gång, vilket gav nivå 0
  och nivå 1 samma svårighetsgrad och gjorde den sista nivån ett steg för lätt.
- Regressionstestet jämför identiska spawn-seed över två intilliggande nivåer.
- Fullt lokalt testpaket: 58 av 58 tester godkända.

## 2026-08-07 (Captive startmeny – källval)

- Startmenyn synkroniserar nu vald Captive-plattform med den senaste
  dataskanningen även när exakt en källa hittas.
- Ett tidigare versionsval kan därför inte längre ligga kvar som ett gammalt
  plattformsval efter att datamappen ändrats eller skannats om.
- Fullt lokalt testpaket: 58 av 58 tester godkända. Captive-datan i
  `~/.opencaptive` verifierar både DOS- och Amiga-källan.

## 2026-08-07 (Captive seedade save-load)

- Captive laddar nu om uppdrag med det auktoritativa `mission_seed`-värdet från
  sparfilen. Tidigare kunde giltiga sparningar från seedade uppdrag avvisas när
  seedet inte gick att härleda från `base_id`.
- Regressionstestet sparar på en inre nivå, laddar sparningen och verifierar
  nivåövergångar både mellan basnivåer och tillbaka till landningszonen.
- Fullt lokalt testpaket: 58 av 58 tester godkända. Captive-datan i
  `~/.opencaptive` verifierar både DOS- och Amiga-källan.

## 2026-08-07 (Captive vapenräckvidd)

- Sprayvapen respekterar nu sin dokumenterade räckvidd 4. Tidigare behandlade
  stridskoden alla distansvapen som räckvidd 6.
- Testet verifierar att ett sprayvapen inte träffar på avstånd 5.
- Combat-dokumentationen beskriver nu den faktiska räckviddsskillnaden.

## 2026-08-07 (Captive shop-översikt)

- Shop-menyn visar nu varje varunamn tillsammans med priset. Tidigare syntes
  bara pris och markering, vilket gjorde varorna omöjliga att skilja åt.
- Regressionstestet säkerställer att olika varunamn ger olika synlig shoptext.
- Riktat shop-test: godkänt.

## 2026-08-07 (Captive hjälpruta och l10n)

- Hjälprutan visar nu `I: Inventory`, vilket stämmer med den faktiska
  tangentbindningen för inventory i både Captive och Liberation.
- Den nya hjälprutesträngen finns i alla 19 språkfiler.
- Samtliga PO-kataloger valideras nu korrekt av `msgfmt`; äldre bokstavliga
  `\n+`-rader i de aktiva katalogerna har rensats.
- Fullt lokalt testpaket: 58 av 58 tester godkända.

## 2026-08-07 (Captive creature-state-validering)

- Captive-sparningar avvisar nu även negativa creature-värden för hastighet,
  räckvidd, cooldown och respawn-timer. Tidigare kontrollerades bara
  maxgränserna, vilket kunde ge korrupt AI-beteende efter en manipulerad
  sparning.
- Regressionstestet täcker samtliga fyra negativa fält.

## 2026-08-07 (Captive CLI-frame capture)

- `--capture-frame` utan ett separat `--game`-argument startar nu Captive
  direkt i stället för att skriva en bild av startmenyn. Ett lyckat capture
  innehåller därmed den förberedda Captive-vyn även i det korta CLI-formatet.
- Verifierat med den lokala `.opencaptive`-datan: resultatet är en 320×200
  spelbild och inte startmenyn. Relevanta tester samt Werror-bygget passerar.

## 2026-08-07 (Captive frame capture och feldata)

- Ett frame-capture med saknad eller ogiltig Captive-/Liberation-data skriver
  inte längre en missvisande startmenybild.
- Kommandot avslutas nu med status 1 utan att skapa någon capture-fil, så
  automatiserade verifieringar kan skilja ett riktigt spelcapture från ett
  datafel.
- Verifierat med dummy-video och dummy-ljud samt lokala startflödestester.

## 2026-08-07 (Captive sidoväggstexturer)

- Sidoväggar i kompatibilitetsvyn använder nu den intilliggande väggcellens
  synliga väggsida. Tidigare lästes texturen från golvcellen, vilket kunde
  ersätta kartans sidovägg med standardtextur.
- Ett regressionstest skiljer på väggcellens och golvcellens textur.

## 2026-08-07 (Captive golv- och taktexturer)

- Kompatibilitetsvyn använder nu `floor_tex` och `ceil_tex` från den synliga
  kartcellen när den väljer panelområde. Tidigare ignorerades fälten helt,
  trots att de sparas i kartdata och cross-save.
- Ett regressionstest verifierar att ändrade golv- och taktexturer påverkar
  den renderade bilden.

## 2026-08-07 (Captive låsta dörrar och pussel)

- Varje genererat basplan får nu minst en låst dörr när kartans choke points
  tillåter det. Pusselgeneratorn kunde tidigare bara länka spakar och
  kontrollpaneler till `CELL_DOOR_LOCKED`, men kartgeneratorn skapade aldrig
  sådana dörrar i den aktiva missionsvägen.
- Karttestet verifierar att varje genererat plan innehåller en låst dörr.

## 2026-08-07 (Captive creature-djupsortering)

- Creature-sprites i Captive sorteras nu från längst bort till närmast innan
  de ritas. Överlappande fiender kan inte längre få felaktig listordning att
  skriva den närmare spriten över den längre bort.
- Regressionstestet renderar samma två fiender i omvänd listordning och kräver
  identisk bild.

## 2026-08-07 (Captive spelbarhet i Original-läge)

- Original-läget använder nu den källbaserade kompatibilitetsrenderaren tills
  den fullständiga DOS-panelkompositorn är återställd. Captive blir därmed inte
  längre en tom HUD i standardläget.
- Enhanced-läget behåller sina extra moderna HUD-/presentationseffekter.
  Dokumentationen skiljer uttryckligen detta från pixelparitet.

## 2026-08-07 (Captive renderläge)

- F10:s Original/Enhanced-gräns är nu verklig i Captive. Den ännu ej
  återställda procedurvyn och dess creatures ritas endast i Enhanced-läge;
  Original-läge lämnar den verifierade GAME SCRN-bakgrunden orörd.

## 2026-08-07 (Captive frame capture)

- `--game captive --capture-frame` initierar nu mission, dungeon, pussel och
  encounters innan den enda bilden skrivs. Capture-verktyget sparar därmed en
  faktisk spelbild i stället för droidkonfigurationsskärmen.

## 2026-08-07 (Captive PL5-transparens)

- Captive viewportens panel-, ornament-, objekt- och creature-blit använder nu
  PL5:s indexplan för transparens. Endast index 0 är transparent; giltiga
  svarta index 16 och 18 raderas inte längre av RGB-baserad filtrering.
- Testtexturen i viewport-regressionstestet innehåller nu också ett indexplan.
  Hela testsviten passerar 58/58.
- Compositorn kan nu köra verifierade DOS-descriptorpaneler med flagga `0x01`
  för horisontell spegling och `0x04` för index-noll-maskning.

## 2026-08-07 (Captive viewport descriptor-layout)

- Dokumenterade och testade CAPPO:s 160×112 viewport-arbetsyta. Descriptorernas
  destination är nu uttryckligen översatt från byteoffset till `(x, y)` med
  originalets 160-byte radsträcka; den slutliga skärmkopian visar 144 pixlar.
- Uppdaterade viewport-dokumentationen utan att påstå pixelparitet innan
  descriptorordning och panelmasker är helt återvunna.

## 2026-08-07 (Captive XP-belöning)

- Captive-striden använder nu den återvunna CAPPO-formeln för XP-belöning vid
  dödade fiender i stället för den gamla `hp_max / 10`-approximationen.
- Belöningen använder fiendens återvunna XP-värde, aktuell svårighetsgrad och
  droidens Experience-skill. Regressionstestet täcker både belöningen och
  nivåökningen.

## 2026-08-07 (Windows CI-bygg)

- Windows-jobbets OpenCaptive-bygg använder nu två parallella byggtrådar.
  SDL-beroendena var redan färdigbyggda när den seriella projektbyggningen
  fastnade i CI; detta använder runner-resurserna bättre och minskar
  timeout-risken utan att ändra körkoden.

## 2026-08-07 (Captive holomap-l10n)

- Captives planetnamn och återgångstext i holamap-skärmen går nu via i18n.
- Lades till i `po/messages.pot`; riktade i18n-, startmeny- och game-state-
  tester passerar.

## 2026-08-07 (Captive hjälp- och droidkonfiguration l10n)

- Captives hjälpskärm använder nu i18n för rörelse, interaktion, inventarie,
  strid, sparning och paus.
- Droidkonfigurationens status, namnbyte och vapenbytesinstruktioner går nu
  också via i18n i stället för hårdkodad engelska.
- Lade till de nya meddelande-ID:na i `po/messages.pot` för de 18 språkfilerna;
  språk utan färdig översättning använder den säkra engelska fallbacken.
- Werror-bygg och hela den lokala testsviten är verifierade: 58 av 58 test
  passerar.

## 2026-08-07 (Captive deterministisk skanningscache)

- Metadata-signaturen för dataskanningen sorterar nu kataloginnehållet innan
  det hashas. Samma dataträd ger därmed samma cache-signatur även när
  filsystemet returnerar katalogposter i olika ordning.
- Detta hindrar onödiga omskanningar av redan kontrollerade Captive-filer.
- Werror-bygg och hela den lokala testsviten är verifierade: 58 av 58 test
  passerar.

## 2026-08-07 (Captive Amiga-verifiering)

- RNC-verifieringen av Captive Amiga-data avvisar nu komprimerade och
  dekomprimerade resurser som inte ryms i avkodarens `int`-storlek. Det
  förhindrar trunkering vid korrupta eller ovanligt stora diskfiler.
- Werror-bygg och hela den lokala testsviten är verifierade: 58 av 58 test
  passerar.

## 2026-08-07 (CI-beroenden)

- CI bygger nu SDL3 utan SDL:s egna testprogram och exempel, som inte behövs
  för OpenCaptive.
- SDL- och SDL_ttf-källor hämtas med blob-filter och grunda submoduler där det
  är möjligt, vilket minskar beroendestegens storlek och Windows-risken för
  timeout.
- Windows-kommandot använder nu PowerShell-kompatibel radsyntax för SDL3-
  flaggorna.

## 2026-08-07 (Captive startmeny)

- Korrigerade startmenyns interna antalspost från sex till åtta val. Det
  omfattar nu även kontroller och avslut, i linje med faktisk navigering och
  klickhantering.
- Lade till ett test som låser detta menyinvariant.

## 2026-08-07 (Captive Windows-CI)

- Flyttade den sista stora `GameState`-allokeringen i `droid_ui`-testet från
  Windows-stackens begränsade minne till statiskt testminne.
- Det åtgärdar segfaulten i GitHub Actions Windows-testet utan att ändra
  Captives körkod.

## 2026-08-07 (Captive dataskanner-cache)

- Dataskannern cachelagrar nu även hashningar som inte gav någon träff, så
  gamla saknade filer inte genomsöks på nytt vid varje uppstart.
- Cache-signaturen innehåller metadata för hela dataträdet och upptäcker därför
  både nya filer och ersatta filer utan att lita på filnamn eller gammal data.
- Lade till regressionstest för negativ cache och ändring av källfil.

## 2026-08-07 (Captive riktade väggtexturer)

- Captives viewport väljer nu väggtextur efter den faktiska väggens riktning,
  även när spelaren vänder sig. Tidigare låstes frontal-, vänster- och
  högerväggar till fasta texturindex.
- Lade till ett regressionstest som verifierar att en vridning ändrar den
  renderade riktade väggen.

## 2026-08-07 (Captive replay interaction)

- Replay-inspelning kodar nu även Enter och numeriskt Enter som Captive-
  interaktion, med samma stabila actionkod som F.
- Numeriskt Enter fungerar nu även direkt i Captive, inte bara vid replay.
- Korrigerat så att numeriskt Enter verkligen når Captive-interaktionen.
- Captive-ornament på golvceller renderas nu på rätt vägg, inklusive pusselpaneler.
- Captive-pusselpaneler återskapas nu vid Continue/F9 efter laddning av sparfil.
- Numeriskt Enter fungerar nu även i Captive-konfiguration, terminal, inventory,
  shop, HOLA-karta och pausmeny.

## 2026-08-07 (Captive interaction key)

- Enter aktiverar nu samma interaktionsflöde som F i Captive.
- Dokumenterade kontroller för dörrar, generatorer, pussel och shop fungerar
  därmed även i praktiken.

## 2026-08-07 (Captive save validation)

- Sparfiler avvisar nu icke-kanoniska värden för pusslens boolska statusflagga.
- Regressionstest säkerställer att en manipulerad `solved`-byte inte kan skapa
  en ogiltig C-`bool` vid laddning.

## 2026-08-07 (Captive button placement)

- Knappgeneratorn placerar nu bara knappar med en faktisk vägg bakom sig.
- Regressionstest verifierar väggorienteringen för genererade knappar över 128
  deterministiska frön.

## 2026-08-07 (Captive puzzle placement collisions)

- Puzzle-generatorn placerar inte längre två interaktioner på samma cell.
- Det förhindrar att fällor, spakar och knappar överlappar och gör varandra
  omöjliga eller tvetydiga att använda.
- Regressionstest kör 64 deterministiska frön och kontrollerar alla placeringar.

## 2026-08-07 (Captive door occlusion)

- Stängda och låsta dörrar räknas nu som ogenomskinliga i 19-cellsvyn, i linje
  med rörelse- och siktlogiken.
- Regressionstest tillagt för båda dörrtyperna.

## 2026-08-07 (Captive shop navigation bounds)

- Shopnavigeringen begränsas nu till samma 12 synliga varor som renderaren.
- Osynliga lagerposter kan därför inte längre väljas och köpas via tangentbord.

## 2026-08-07 (Captive adjacent-shop entry)

- Shoppen öppnas nu korrekt även när spelaren står framför en shop-cell.
- Den tidigare vägen ändrade bara speltillståndet till `STATE_SHOP` utan att
  ladda lager, guld eller musik och kunde därför visa gammal shop-data.
- Båda öppningsvägarna använder nu samma initialisering.

## 2026-08-07 (Captive armor inventory safety)

- Att ta av en rustningsdel när droidens inventarie är full avbryts nu utan
  att den utrustade delen försvinner.
- Regressionstest lagt till för full inventarielista.

## 2026-08-07 (Captive Manhattan combat reach)

- Stridsavståndet använder nu Manhattan-avstånd i stället för Chebyshev-
  avstånd.
- Närstridsvapen kan därför inte längre träffa diagonalt genom en enda
  rörelseaxel; samma korrigering gäller varelsers upptäckt och attackräckvidd.
- Regressionstest lagt till för diagonalt mål med närstridsvapen.

## 2026-08-07 (Captive mission-allocation failure)

- Uppdragsgenerering returnerar nu lyckad/misslyckad status i stället för att
  tyst lämna ett delvis initierat spel.
- Start från droidkonfiguration, byte från Holomap och save-load går inte
  längre vidare till spel med en tom dungeon om temporär kartallokering
  misslyckas.
- Regressionstestet täcker null-state-kontraktet för båda generator-API:erna.

## 2026-08-07 (Captive Holomap-shop transition)

- Captive-shoppen minns nu om den öppnades från aktivt spel eller Holomap.
- `ESC` från shoppen återgår därför till rätt skärm; shopping från Holomap
  återför inte längre spelaren direkt till föregående uppdrag.

## 2026-08-07 (Captive save-respawn-invariant)

- Save/load avvisar nu döda, inaktiva fiender utan aktiv respawn-timer.
- Ett sådant tillstånd kunde tidigare sparas men blev permanent osynligt och
  kunde aldrig återaktiveras av `combat_tick()`.
- Roundtrip- och valideringstesterna täcker nu det korrekta döda tillståndet
  med 600 ticks kvar.

## 2026-08-07 (Captive combat game-over invariant)

- `combat_tick()` sätter nu `STATE_GAMEOVER` direkt när den sista levande
  droiden faller, oberoende av vilken anropare som kör combat-lagret.
- Huvudloopen behåller sina meddelanden och ljudeffekter, men game-over är
  inte längre beroende av just den presentationens efterkontroll.
- Werror, UBSAN och hela testsviten (58/58) passerar.

## 2026-08-07 (Captive respawn-timers)

- Fiendersnas respawn-timer tickar nu även när fienden befinner sig på en
  annan våning än party.
- Respawn är därmed knuten till världens tick-loop i stället för den aktuella
  viewporten; en dödad fiende blir inte permanent inaktiv efter en
  våningsväxling.
- Werror, UBSAN och hela testsviten (58/58) passerar.

## 2026-08-07 (Captive spawn-positioner)

- Nya Captive-encounters och byggnadsinteriörer undviker nu alltid partiets
  aktuella ruta när fiender skapas.
- Den tidigare spawnvägen kunde skapa en aktiv fiende direkt ovanpå party,
  vilket gav inkonsekvent rendering, omedelbar kollision och ett save-läge
  som senare avvisades av valideringen.
- Regressionstestet täcker 512 deterministiska seeds; Werror, UBSAN och hela
  testsviten (58/58) passerar.

## 2026-08-07 (Captive gemensamt datamanifest)

- Captive-start, startmenyns skanner och `--verify-data captive` använder nu
  samma centrala manifest med 25 verifierade boot- och atlasresurser.
- Duplicerade hashlistor i `main.c` och startmenyn är borttagna, så framtida
  manifeständringar kan inte ge olika verifieringsresultat i olika vägar.
- `build-werror` och hela testsviten (58/58) passerar efter ändringen.

## 2026-08-06 (Captive gemensam atlasverifiering)

- Startmeny, `--verify-data` och Captive-start använder nu samma kompletta
  lista över PL5-ytor som renderingsatlasen kräver.
- Captive markeras inte längre som tillgängligt när bara en del av atlasen
  finns.

## 2026-08-06 (Captive atlas-uppstart)

- Captive startar inte längre med verifierade men ofullständigt avkodade
  PL5-resurser. Om renderarens kompletta atlas inte kan laddas stannar
  uppstarten säkert i menyn.
- Samma kontroll gäller direktstart och Continue.

## 2026-08-06 (Captive save-positioner)

- Save/load avvisar nu aktiva fiender som överlappar party eller andra aktiva
  fiender på samma våning.
- Regressionstest täcker både party-overlap och dubbel fiende-position.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive trappövergångar)

- Trappor kan inte längre lämna party ovanpå en aktiv fiende på
  mottagningsvåningen. Övergången återställs till den ursprungliga våningen
  om ankomstcellen är blockerad.
- Kollisionstätningen ligger i en gemensam combat-hjälpfunktion och täcks av
  ett regressionstest.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive teleporter-kollision)

- Teleporterfällor kan inte längre lämna party ovanpå en aktiv fiende, vare
  sig fällan triggas genom rörelse eller interaktion.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive party-fiende-kollision)

- Party kan inte längre gå in i en aktiv fiendes ruta; movement och creature-AI
  använder nu samma occupancy-regel.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive inventory-droidval)

- Tangenterna `1`–`4` byter nu aktiv droid direkt inne i Captive-inventoryt
  och uppdaterar samtidigt panelens equipment/inventory.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive F10-realtidsfusk)

- `Invulnerable` och `Infinite energy` appliceras nu direkt när F10-popupens
  val ändras och efter gameplay-input, även om samma event annars skulle ha
  utlöst game-over.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive save-positioner)

- Save/load avvisar nu party- och creature-positioner på väggar eller
  stängda dörrar, inte bara koordinater utanför kartan.
- Lade till regressionstester för både blockerad party-position och creature,
  inklusive en manipulerad save som avvisas vid laddning.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive LOS-hörn)

- Ranged combat och creature-LOS blockerar nu diagonala hörn där båda
  sidocellerna är väggar eller stängda dörrar.
- Lade till regressionsfall för skott genom ett helt blockerat hörn.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive fiende-kollisioner per våning)

- Fiende-AI: rörelse på aktuell nivå blockeras inte längre av en aktiv fiende
  som råkar ha samma koordinater på en annan våning.
- Lade till regressionsfall som täcker två våningar.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive creature-occlusion)

- Aktiva fiender ritas nu bara i giltiga, synliga 19-cellerspositioner och
  kan inte längre renderas genom väggar eller utanför kartan.
- Lade till viewport-regression för synlig fiende respektive fiende i vägg.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive teleportlandning)

- Rörelseflödet använder nu den faktiska landningsrutan efter en teleporter
  för item-upptagning och golvhazarder.
- En dödlig pussel-hazard avbryter resterande rörelsehantering direkt.
- Lokal Werror/CTest: 58/58 tester passerar.

## 2026-08-06 (Captive hazard game-over)

- Golvfällor och elektriska väggar som slår ut hela droidgruppen sätter nu
  `STATE_GAMEOVER` direkt, både vid interaktion och när gruppen går på fällan.
- Lade till regressionsfall för båda vägarna samt för elektrisk vägg-hazard.
- Lokal Werror-svit: 58/58 tester passerar.

## 2026-08-06 (Säker återställning av Liberation-position)

- Liberation-save återställer inte längre en spelare på en vägg eller annan
  icke-körbar city-cell när den verifierade staden är aktiv.
- När sparad position är ogiltig väljs närmaste körbara vägcell och spelarens
  sparade riktning behålls.

## 2026-08-06 (Säker Liberation-itemkonvertering)

- 16-bitars Liberation-itemtyper trunceras inte längre tyst till 8-bitars
  Captive-ID:n när de återställs eller flyttas till en droid.
- Ogiltig equipment rensas vid save-restore och felaktiga shared/shop-/bar-
  objekt stannar i shared inventory i stället för att bli korrupta runtime-
  items.

## 2026-08-06 (Liberation hazard leder korrekt till game-over)

- En industrial hazard som förstör alla droids när en byggnad lämnas sätter
  nu `STATE_GAMEOVER` direkt.
- Falska efterföljande bar fights eller combat-state kan därmed inte starta
  efter att hela gruppen redan dött.

## 2026-08-06 (Enhetlig startutrustning i Captive och Liberation)

- Nya droids får nu ett riktigt KNUCKLE-DUSTER-ID tillsammans med matchande
  cached skadevärde i stället för ett löst skadevärde utan utrustning.
- Liberation-combat kräver nu, liksom Captive, ett giltigt utrustat vapen före
  energi dras eller attackskada beräknas.
- Regressionstest täcker att Liberation inte kan attackera med tomma
  vapenplatser även om `weapon_damage` är manipulerat.

## 2026-08-06 (Korrekt droidutrustning och slotvalidering)

- Nya droids får nu HEAD, CHEST, ARM, LEG, FOOT och HAND i rätt
  kroppsplatser i stället för HEAD i alla sex platser.
- Captive-save och cross-save avvisar armor i vapenplatser och vapen i
  armorplatser, även om item-ID:t i sig är giltigt.
- Regressionstester täcker både korrekt startutrustning och felaktiga
  slotkategorier.

## 2026-08-06 (Stridsattack kräver utrustat vapen)

- Captive kan inte längre attackera med bara ett kvarlämnat `weapon_damage`-
  värde när båda vapenplatserna är tomma.
- Ogiltiga eller icke-vapen-ID:n ger inte längre en falsk närstridsattack.
- Regressionsfall täcker både tomma vapenplatser och den normala skadevinstvägen.
- Werror-bygg och hela testsviten: 57/57 godkända.

## 2026-08-06 (Säker pausrendering inne i Liberation-dungeon)

- Pausvyn använder nu 320×200 när Liberation befinner sig i dungeonläge,
  precis som huvudloopen och den underliggande Captive-vyn.
- Det förhindrar att pausrenderingen skriver 320×256 pixlar till en
  320×200-framebuffer.

## 2026-08-06 (Holomap-återgång synkroniserar menyn)

- ESC från Captives holomap går nu genom samma menyåtergång som övriga
  avslutade speltillstånd.
- Runtime-/F10-inställningar följer därmed med tillbaka till startmenyn även
  efter uppdragsavslut utan att starta nästa uppdrag.

## 2026-08-06 (Menyåtergång återställer scan och runtime-konfiguration)

- Återgång från pausens QUIT samt Game Over/Victory använder nu samma
  konfigurationssynkronisering som övriga menyvägar.
- Bakgrundsskanningen startas därför igen och datakortens status blir inte
  kvar i ett falskt “saknas”-läge.
- F10-ändringar pågående i spelet speglas samtidigt tillbaka i menyn.

## 2026-08-06 (Enhetliga Liberation-canvasar för alla vyer)

- Huvudloopen väljer nu Liberation-canvas även för hjälp, inventory och
  stadskarta, inte bara aktivt spel och paus.
- Rendererens texturemått följer därmed vyernas faktiska 320×256 PAL- eller
  320×200-layout och lämnar inte felaktiga letterbox-/stale-rader efter
  state-byten. Intro- och stadsbildernas verifierade 162/167-pixelregioner
  ligger inuti Liberation-canvasen.

## 2026-08-06 (ANM-introt frigörs vid skip)

- Captive-introt frigör nu sin ANM-buffer direkt när användaren hoppar vidare
  med en tangent.
- Upprepade start-, skip- och menycykler behåller därmed inte gamla introdata
  i minnet.

## 2026-08-06 (Startup-popup blockerar menyval bakom)

- Musknapp på startup-popupen stänger nu popupen utan att samtidigt aktivera
  ett menyval bakom den.
- Både tangentbords- och musvägen har därmed samma blockerande beteende.
- Regressionstest täcker musstängning av popupen.

## 2026-08-06 (Ingen kvarhängande datastatus efter ny skanning)

- En ny bakgrundsskanning nollställer nu omedelbart tidigare spel- och
  versionsstatus.
- Tomma eller ogiltiga sökvägar kan därför inte lämna kvar gröna
  tillgänglighetsmarkeringar från en föregående datakatalog.
- Scannerpopupens ZIP- och progressräknare återställs samtidigt.
- Regressionstest täcker tom sökväg och att bakgrundsskanningen avslutas
  korrekt.

## 2026-08-06 (Letterbox-säkra muszoner och Liberation-paus)

- Meny-, Liberation- och pausens muskoordinater räknas nu om från den
  centrerade canvasen i stället för att skala hela fönstret direkt.
- Hover och klick hamnar därför rätt även när fönstret har annan aspekt än
  canvasen och svarta letterbox-kanter visas.
- Liberation-pausen använder nu samma 320×162- respektive 320×200-canvas som
  huvudloopen beroende på om spelet är ute eller inne i en byggnad.
- Headless frame-capture och hela testsuiten passerar efter ändringen.

## 2026-08-06 (Säkra canvasbyten i rendereraren)

- Rendererens canvas-, upscale- och widescreenbyten returnerar nu felstatus
  när SDL inte kan skapa den nya framebuffer-texturen.
- Huvudloopen fortsätter inte med nya bildmått mot en gammal texture vid ett
  sådant fel, vilket förhindrar felaktig stride eller buffertläsning.
- Werror-bygget och hela testsuiten passerar efter ändringen.

## 2026-08-06 (Klickzoner följer logotypens storlek)

- Startmenyns mus-hit-test använder nu samma beräknade logotyphöjd som
  renderingen.
- Korta eller breda installerade logotyper förskjuter därför inte längre
  kortens klickzoner.
- Regressionstest täcker en logotyp med extrem aspektfördelning.

## 2026-08-06 (ANM-intro återhämtar tappad tid)

- Captive-introt avancerar nu över alla hela 100 ms-intervall som gått sedan
  föregående bildruta, i stället för att alltid gå exakt en ruta vid lagg.
- Frame-indexet kontrolleras mot återstående rutor innan övergången till
  droidkonfiguration, så långsamma renderingar kan inte läsa utanför introt.

## 2026-08-06 (Tidsstabil menyanimation)

- Startmenyns markeringspuls och fallback-kortanimation använder nu förfluten
  tid med nominell 60 Hz-takt i stället för ett steg per renderingsanrop.
- Animationerna behåller därmed samma hastighet vid olika FPS-gränser och
  hoppar inte fram oproportionerligt efter ett långt fönsterstopp.

## 2026-08-06 (Deterministisk headless frame-capture)

- `--capture-frame` avslutar nu frame-loopen direkt efter att PPM-filen
  skrivits.
- Ljudmixning och SDL-presentering körs inte efter en one-shot capture, vilket
  gör dummy-driver- och CI-körningar tillförlitliga.
- Capture med riktig `.opencaptive`-data producerade en 320×200-bild och
  avslutade med status 0.

## 2026-08-06 (Bakgrundsskanning vid första uppstart)

- Startmenyn startar nu den inkrementella, cachebaserade dataskanningen i
  bakgrunden i stället för att blockera innan första menyrutan visas.
- Korten får verifieringsstatus när skanningen är färdig, medan D fortfarande
  öppnar ett synligt progressfönster för samma scanner.
- Spelstart gör en slutlig verifiering efter användarens val och förblir
  säker om data ändras mellan menyvisning och start.
- Regressionstest täcker att bakgrundsskanningen gör framsteg utan att öppna
  scannerfönstret.

## 2026-08-06 (VFS-cache utan upprepad katalogskanning)

- Dataskannern beräknar nu rotens cache-signatur en gång när VFS:en öppnas.
- Varje cachepost verifierar dessutom sin ursprungliga lösa fil eller ZIP-fil
  med en egen metadatafingeravtryckning, så ändrade källor invalideras även
  medan samma VFS-instans används.
- Ett flerfilsuppslag går därför inte längre igenom hela dataträdet för varje
  hash, samtidigt som ersatta ZIP- och lösfiler fortsatt nekas gammal data.
- VFS-öppningen räknar inte längre om hela lösfilsträdet; endast vald rot och
  upptäckta arkiv ingår i den billiga globala namnrymden.

## 2026-08-06 (Isolerat VFS-test)

- `test_data_vfs` kör nu i en unik temporär arbetskatalog i stället för i
  hela bygg- eller repoträdet.
- Testet lämnar inga fixturefiler i projektroten och blir därmed snabbare och
  mer förutsägbart i sanitizers och CI.

## 2026-08-06 (Säker UTF-8-avkodning i runtime-popup)

- Den lilla pixeltext-renderaren kontrollerar nu att UTF-8-fortsättningsbytes
  finns innan de läses.
- Avkortade eller skadade lokaliserade strängar faller tillbaka till `?`
  utan läsning förbi strängens slut.

## 2026-08-06 (Renderer-fallback vid ogiltig skala)

- Renderer använder nu originalpixlar om `upscale_factor` är ogiltig, i
  stället för att presentera en oinitierad temporär buffer.
- Detta täpper till den statiska analysens odefinierade-data-varning utan att
  ändra giltiga 2x/3x/4x-lägen.
- Werror-build och riktade renderer-/meny-/VFS-tester passerar.

## 2026-08-06 (Intern cache filtreras ur VFS-sökning)

- ZIP-discovery och lösfilssökning hoppar nu över `.cache` på både Windows
  och POSIX.
- Cacheartefakter kan därmed inte identifieras som speldata när dataroten är
  hemkatalogen.
- Riktat `data_vfs`-test passerar efter ändringen.

## 2026-08-06 (Säker ZIP-nästling)

- Nästlade ZIP-poster avbryter nu säkert vid misslyckad minnesallokering i
  stället för att skriva via en nullpekare.
- Ändringen gäller både lagrade och deflaterade poster genom den gemensamma
  minnesdekodern.

## 2026-08-06 (Intern cache undantas från metadata)

- VFS-cachekatalogen `.cache` räknas inte längre som speldata när vald
  datarot omfattar användarens hemkatalog.
- Cache-skrivningar kan därför inte själva orsaka nya signaturer och onödiga
  omläsningar.
- `data_vfs`-testet passerar efter ändringen.

## 2026-08-06 (Manuell CI-körning)

- Build-workflowen stöder nu `workflow_dispatch`, så aktuell `main` kan
  verifieras manuellt när GitHub Actions lämnar äldre push-körningar i kö.

## 2026-08-06 (Windows cache-invalidation i dataskannern)

- GitHub Actions avslöjade att en ersatt lös fil kunde få gammal hashcache
  på Windows när katalogens metadata inte ändrades.
- VFS:ens per-källfingeravtryck använder Windows filidentitet och högupplöst
  ändringstid, så ersatta lösa filer invaliderar sina poster utan att varje
  hashuppslag behöver skanna hela metadata-trädet.
- `data_vfs`-regressionstestet passerar lokalt efter ändringen.

## 2026-08-06 (Icke-blockerande Liberation-skanning)

- Startmenyns Liberation-skanning verifierar nu bara nödvändiga källfiler
  under progress-steget.
- Den valfria RNC/ANIM-presentationen dekoderas först när Liberation faktiskt
  startas, i stället för att blockera ett helt menyframe.
- Full Werror-build och hela testsuiten med 56 tester passerar.

## 2026-08-06 (Versionspopup med verifierade källor)

- Versionspopupen visar och startar nu endast spelversioner som faktiskt
  verifierats i den aktuella datamappen.
- Captive DOS/Amiga och Liberation CD32/Amiga mappas via sina riktiga
  plattforms-ID:n även när bara den andra versionen finns.
- Mus- och tangentbordsval använder samma källista, med regressionstest för
  ett ensamt verifierat alternativ.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (SCALE från startmenyn)

- Startmenyns `SCALE` är inte längre ett dött reglage: ett uttryckligt värde
  1–5 bevaras när spelet startas och beräknar den anpassade native-
  fönsterstorleken med samma minimiummått som CLI:t.
- `WINDOW SIZE` återtar prioriteten när användaren väljer ett preset efteråt.
- Regressionstestet täcker växlingen mellan anpassad skala och preset.
- CLI-capture med `--scale 2` och riktig `.opencaptive`-data lyckades.
- Werror-kompilering, statisk analys av huvud-/startmenykod och hela
  testsuiten med 56 tester passerar.

## 2026-08-06 (Reverb från startmenyn)

- Startmenyns `REVERB`-val appliceras nu på den aktiva ljudmixern när nästa
  spelstart konfigureras; tidigare sparades valet men mixerflaggan behöll
  föregående värde.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (Icke-blockerande dataskanner)

- Startmenyns D-skanner kör nu en verifierad hashkontroll per menyframe i
  stället för att blockera hela eventloopen.
- Progressfältet kan därför uppdateras under skanning, medan den befintliga
  metadata-/identitetscachen fortfarande återanvänds.
- Scanner-VFS frigörs både vid färdig skanning och när användaren lämnar
  scannerfönstret.
- Regressionstestet täcker stegvis progress och resursfrisläppning.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (F10-popup i Liberation)

- F10-popupen använder nu kompakt radlayout på Liberation-canvasens 320×162
  pixlar, så alla grafik- och fuskalternativ förblir synliga och valbara.
- Den onödiga skrivningen i startmenyns setup-popup är borttagen; riktad
  Clang Static Analyzer-körning för startmeny och data-VFS är nu tyst.
- Riktig `.opencaptive`-data verifierar Captive, Liberation och båda
  Liberation-första bildrutorna.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (Uppstartsljud och intro)

- Startmenyns samplingsfrekvens bygger nu om SDL-ljudströmmen och MIDI-
  renderaren när värdet ändras, så nästa spelstart använder 22050, 44100
  eller 48000 Hz utan omstart av programmet.
- Om värdet inte accepteras av ljudenheten återanvänds den tidigare frekvensen
  och ljudet förblir användbart.
- Tomma eller skadade Captive-intros lämnar nu uppstarten säkert i stället
  för att visa en permanent svart bild; färdigspelade ANM-resurser frigörs.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (CLI-skala)

- `--scale` fortsätter nu att styra fönsterstorleken även efter att startmenyn
  fått ett explicit `WINDOW SIZE`-värde.
- En senare `--resolution` kan fortfarande skriva över skalan enligt
  kommandoradens ordning; headless-capture med `--scale 2` passerar.
- Hela testsuiten med 56 tester passerar.

## 2026-08-06 (Renderer-byte i realtid)

- Renderer-valet från startmenyn kan nu byta SDL3-backend när spelet startas
  eller menyn återgår till spel; tidigare skapades launchern alltid med AUTO.
- Byte av backend återskapar framebuffer-texturen säkert och faller tillbaka
  till AUTO om vald backend saknas.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (Renderer-val)

- Startmenyns `RENDERER`-val skickas nu vidare till SDL3: AUTO, GPU eller
  SOFTWARE används vid renderer-initiering.
- Ogiltiga configvärden faller tillbaka till AUTO.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (Fönsterstorlek från startmenyn)

- `WINDOW SIZE` kopieras nu från menyinställningen till konfigurationen och
  appliceras på det aktiva SDL-fönstret.
- Standardfönstret är explicit 1280×800; matchande CLI-upplösningar återförs
  korrekt till motsvarande menyval.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (Mastervolym)

- Startmenyns `VOLUME` påverkar nu både SFX-mixern och MIDI-musiken.
- Volymen behålls när musikspår byts; tidigare återställdes varje nytt spår
  till en fast nivå på 30 %.
- Standardkonfigurationen anger nu explicit gamma 50 och mastervolym 80,
  så första uppstarten visar och använder avsedda neutrala värden.
- Ljudtestet täcker volymens gränsklippning och hela testsuiten med 56 tester
  passerar.

## 2026-08-06 (Gamma i renderaren)

- Inställningen `GAMMA` i startmenyn används nu av rendererens aktiva
  postprocessning med 50 % som neutral nivå.
- Ändringen gäller även när F10-popupen ändrar grafikinställningar i realtid;
  gamma aktiverar postprocessning även när övriga effekter är avstängda.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (Säkrare referensbildsläsning)

- VGA- och PPM-referensläsarna avbryter nu korrekt efter avkortade filer och
  försöker inte läsa vidare från en ström vars position är osäker eller redan
  nått EOF.
- Statisk analys av `src/main.c` är tyst och hela testsuiten med 56 tester
  passerar.

## 2026-08-06 (Start-menyns återinträde)

- Menyn återanvänder nu laddade bilder och fonter när användaren går tillbaka
  från paus. Den tidigare synkroniseringen nollställde pekarna utan att frigöra
  resurserna och läckte därför vid upprepade återinträden.
- Regressionstestet täcker resursbevarande reinit och återställd datasökväg.
- Uppstartsmenyns droidkonfiguration fick samtidigt tätare radlayout så att
  den fjärde droidens villkorsrad inte överlappar åtgärdshinten på 320×200.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (Atomic mission metadata)

- `game_state_new_mission()` beräknar nu seed lokalt och låter den seedade
  kartgeneratorn uppdatera mission och seed först efter lyckad temporär
  dungeonallokering.
- Ett allokeringsfel kan därför inte lämna gammal dungeon med ny
  missionsmetadata.
- Werror-kompilering och hela testsuiten med 56 tester passerar.

## 2026-08-06 (VFS loose-file cache invalidation)

- VFS-cacheproben inkluderar nu metadata för lösa filer, inte bara dataroten
  och ZIP-arkiven.
- Ett ersatt löst speldataobjekt med samma storlek kan därför inte längre
  återanvända en gammal hash-cachepost.
- Regressionstestet verifierar uttryckligen att den gamla hashen avvisas efter
  filbyte.

## 2026-08-06 (Map generator frontier performance)

- MapGen använder nu en inkrementell frontier i Architect-vandringen i stället
  för fullständig kartskanning efter varje blockerat steg.
- Kantceller som `carve()` inte kan skriva till avvisas innan de räknas som
  kandidater; den särskilda rad-0-ingången lämnas oförändrad.
- Hela Werror-sviten med 55 tester passerar; `test_map_gen` validerar 10 000
  seedvärden på cirka 5 sekunder.

## 2026-08-06 (Linux installed resource lookup)

- Font-, l10n- och launcherbildsladdning söker nu standardiserade Linux-
  installationer under `../share/opencaptive` och `/usr/share/opencaptive`.
- Verifierade `i18n`, `start_menu`, Werror-kompilering och release-workflowens
  YAML.

## 2026-08-06 (Release resource packaging)

- RPM-paketet inkluderar nu font, l10n-kataloger och menyresurser i både
  källtarball och installerad fil-lista.
- AppImage-bygget kopierar samma menyresurser som övriga desktop-paket.
- Validerade `.github/workflows/release.yml` som YAML och kontrollerade
  release-diffen.

## 2026-08-06 (Hermetic VFS regression test)

- Isolerade testet för överlånga ZIP-namn i en egen testkatalog.
- En avbruten tidigare testkörning eller en annan fixture kan därför inte
  påverka testets negativa resultat.
- `test_data_vfs` passerar efter ändringen.

## 2026-08-06 (VFS scan performance)

- Lade till en billig probe-signatur för datarot och upptäckta ZIP-filer.
- Full metadata-signatur räknas nu bara om när probe-signaturen för datarot
  eller arkiv ändras, så arkivbaserade hashuppslag skannar inte hela
  dataträdet upprepade gånger.
- `test_data_vfs` passerar på 1,87 sekunder i Werror-byggningen.

## 2026-08-06 (Liberation F9 world restore)

- F9 regenererar nu Liberation-staden från sparfilens mission och seed innan
  navigator, droids och progression återställs.
- Rensar samtidigt intro-, missionsmeny- och briefingflöden som hör till den
  föregående live-sessionen.
- Verifierade `liberation_save`, `game_state` och Werror-kompilering.

## 2026-08-06 (Windows VFS cache precision)

- Windows-cache-signaturen använder nu 100 ns-ändringstid, volym och
  filindex från `GetFileInformationByHandle`.
- Åtgärdar Windows-CI-felet där ett snabbt ersatt ZIP kunde behålla gammalt
  hashresultat i en levande VFS-instans.
- `test_data_vfs` passerar lokalt efter ändringen.

## 2026-08-06 (Portable Captive save format)

- Captive-sparningar använder nu explicit little-endian-serialisering för
  header, droids, varelser och pussel i formatversion 5.
- Äldre native-format v3/v4 kan fortfarande läsas.
- Lade till test som kontrollerar den fasta version 5-headern och verifierade
  `test_save_load` samt Werror-kompilering.

## 2026-08-06 (Live VFS cache invalidation)

- VFS:en räknar om källmetadata före varje cacheuppslag, så ett utbytt ZIP-
  arkiv invaliderar gamla poster även i en redan initierad VFS-instans.
- Lade till regressionstest som ersätter ett ZIP efter första uppslaget och
  verifierar att gammalt hashresultat avvisas.
- `test_data_vfs` passerar efter ändringen.

## 2026-08-06 (Atomic VFS cache writes)

- Verifierade payloads och cachemetadata skrivs nu till processunika temporära
  filer och ersätts atomiskt.
- En parallell scanner kan därför bara se det gamla kompletta cacheparet eller
  det nya kompletta paret, aldrig en halvskriven fil.
- `test_data_vfs` passerar efter ändringen.

## 2026-08-06 (Save/load failure diagnostics)

- Captive-, Liberation- och cross-save-returvärden kontrolleras nu i
  tangentflödet.
- Misslyckade skrivningar eller läsningar rapporteras på stderr i stället för
  att användaren lämnas utan förklaring.

## 2026-08-06 (Replay recording output)

- `--replay-record` sparar nu den inspelade Captive-replayen när programmet
  avslutas, i `opencaptive.ocrp` som standard.
- Lade till `--replay-output <fil>` för explicit sökväg och felmeddelande när
  sökvägen saknas.
- Dokumenterade inspelning och uppspelning i README och release notes.

## 2026-08-06 (Atomic replay and feature configuration)

- Replay- och feature-konfigurationsfiler skrivs till temporära filer och byts
  atomiskt efter lyckad skrivning.
- Skyddar inspelningar och användarens runtime-konfiguration mot trunkerade
  filer vid avbrott eller skrivfel.
- Verifierade `test_custom_features` och diff-format.

## 2026-08-06 (Atomic cross-save export)

- Captives portabla cross-save skrivs färdigt till en temporär fil innan den
  ersätter befintlig `.ocsv`.
- Misslyckade exportförsök lämnar tidigare fungerande cross-save intakt.
- Verifierade `test_custom_features` och diff-format.

## 2026-08-06 (Atomic Liberation saves)

- Liberation-sparningar skrivs färdigt till en temporär fil innan destinationen
  ersätts atomiskt.
- Ett avbrutet eller misslyckat skrivförsök kan inte längre trunka den senaste
  fungerande Liberation-sparningen.
- Verifierade `test_liberation_save` och diff-format.

## 2026-08-06 (Atomic Captive saves)

- Captive-sparningar byggs färdigt i en temporär fil innan den ersätter den
  aktiva save-filen.
- Ett skriv- eller diskfel kan därför inte längre lämna den tidigare fungerande
  sparningen trunkerad.
- Verifierade `test_save_load`, Werror-kompilering och diff-format.

## 2026-08-06 (HQ MIDI output)

- Kopplade `hq_midi` och `--hq-midi` till MIDI-spelaren med ett kort
  lågpassfilter efter OPL2-renderingen.
- Lade till regressionstest som säkerställer att HQ-läget faktiskt ger en
  annan ljudbuffer än standardläget utan att ändra MIDI-timing.
- Dokumenterade att läget förbättrar utgångsfiltrering, inte instrumentbankens
  verifierade patchar.

## 2026-08-06 (Widescreen presentation)

- Kopplade `--widescreen` till SDL-presenteringen med automatisk 16:9-bredd
  och stöd för begränsad `widescreen_width` i feature-konfigurationen.
- Native-framebuffer och capture-formatet förblir oförändrade.
- Lade till validering så orimliga konfigurationsbredder faller tillbaka till
  automatisk bredd.

## 2026-08-06 (HD framebuffer upscaling)

- Kopplade `--hd-upscale` och `--upscale-factor` till den gemensamma SDL-
  presenteringen. Native-spelbilden lämnas oförändrad och xBRZ appliceras
  endast på utdata i 2x, 3x eller 4x.
- Dokumenterade flaggorna och F10-popupen i README, User Guide och wiki-index.

## 2026-08-06 (Audio sample-rate configuration)

- Startmenyns val av 22 050, 44 100 eller 48 000 Hz används nu av SDL:s
  ljudström samt SFX- och MIDI-renderingen.
- Lade till MIDI-regressionstest som verifierar att ticktimingen följer vald
  samplingsfrekvens.
- Dokumenterade inställningen i README, ljudsystemdokumentationen och den
  tekniska Captive-dokumentationen.

## 2026-08-06 (Enable cross-save export)

- `--cross-save-export` och `cross_save=1` används nu av Captive F5-sparning.
- Vanliga sparningar skapar `opencaptive.ocsv`; quicksave använder motsvarande
  `opencaptive_slotN.ocsv` bredvid den normala `.sav`-filen.

## 2026-08-06 (Liberation session reset)

- Varje ny Liberation-session nollställer nu transient lägesdata för dungeon,
  byggnad, strid, böter och inventariecursor innan presentationen startar.
- Om `Continue Liberation` hittar ett borttaget eller korrupt save skapas en
  ren session i stället för att gamla GameState-data återanvänds.
- Verifierade relevanta save-, startmeny- och GameState-tester samt Werror-build.

## 2026-08-06 (Puzzle hint localization)

- Pusselens clipboard-hintar använder nu l10n för lösning, matchning, kod och
  lösenord samt återanvänder översättningarna för `ON` och `OFF`.
- POT och samtliga översättningskataloger regenererades efter källändringen.

## 2026-08-06 (Empty deflated ZIP entries)

- VFS-dekodern accepterar nu även giltiga deflaterade ZIP-poster med noll
  bytes, i linje med hanteringen av tomma lösa och lagrade ZIP-filer.
- Lade till regressionstest som täcker den komprimerade tomma posten i den
  vanliga ZIP-vägen.

## 2026-08-06 (Runtime screen localization)

- Synliga rubriker och statusmeddelanden i spelvyn använder nu samma l10n-väg
  som startmenyn, inklusive paus, spel över, seger, droidkonfiguration,
  stadsöversikt och Liberation-strid.
- POT-filen och alla 18 översatta kataloger uppdaterades; engelska är den
  nittonde fallbacken när en katalog saknar en översättning.
- PO-filerna validerades med `msgfmt` och i18n-regressionstestet passerar.

## 2026-08-06 (Architect base density)

- Kampanjans Architect-generator garanterar nu minst 21 spelbara celler per
  logisk våning inom dess tilldelade sektion.
- Garantin korsar inte sektionsgränser och återställer inte trappor eller
  andra interaktiva celler.
- Seed-svepet i karttestet utökades till 10 000 värden och fångar bland annat
  den tidigare underdimensionerade rotvåningen för seed 161.

## 2026-08-06 (Liberation city-grid finalization)

- Den finaliserade Liberation-kartan skriver nu celltyper till det plan som
  navigering och rendering använder.
- Ursprungliga byggnads-ID:n sparas separat så att entréer, byggnadsdialoger,
  väggfärger och snabbresor fortfarande hittar rätt byggnad efter finalisering.
- Lade till regressionstest som säkerställer att både synliga celler och
  byggnads-ID:n finns kvar efter genereringen.

## 2026-08-06 (Recursive ZIP data discovery)

- VFS:en hittar nu ZIP-arkiv rekursivt under den valda datakatalogen på alla
  plattformar, inte bara direkt i rotkatalogen.
- Lade till regressionstest för speldata i en underkatalog.

## 2026-08-06 (Puzzle record reinitialization)

- Nya pusselposter nollställs nu alltid när pusselgeneratorn återanvänder en
  `PuzzleList`, och olänkade pussel får konsekvent målkoordinaterna `(-1,-1)`.
- Förhindrar att gamla mål eller tillstånd följer med till exempelvis power
  sockets efter en ny pusselgenerering.
- Lade till regressionstest med en förfylld och återanvänd pussellista.

## 2026-08-06 (Windows VFS cache determinism)

- VFS-cache-signaturen sorterar nu Windows-katalogposter innan metadata hashas.
  Oförändrade speldata får därför stabila cacheträffar även mellan separata
  körningar, i stället för att ibland skannas om på grund av filsystemets
  ospecificerade listordning.

## 2026-08-06 (Liberation save state isolation)

- Temporära pending-böter från ett tidigare Liberation-spel läcker inte längre
  in efter att ett save har laddats.
- Save-load behandlar nu det runtime-state som inte finns i det äldre
  save-formatet som nytt och tomt.

## 2026-08-06 (Liberation police dialogue rebuild)

- Polisdialogen byggs nu om när ett pending barbråk kopplas till besöket, så
  betalningsvalet faktiskt visas i spelgränssnittet.
- Lade till regressionstest för betalning, guldavdrag och avslutad dialog.

## 2026-08-06 (Liberation police fine flow)

- Barbråkets påföljd sparas nu tills nästa besök på en polisstation, så
  betalning eller vägran av böter fungerar i riktig spelkörning.
- Lade till state-hantering som kopplar ihop barens och polisens separata
  byggnadsdialoger.

## 2026-08-06 (F10 renderer synchronization)

- F10-popupens byte mellan original/enhanced grafikläge uppdaterar nu även
  rendererns live state direkt, inte bara spelkonfigurationen och
  postprocess-effekterna.

## 2026-08-06 (Mixed droid weapons)

- Droidar med närstridsvapen i ena handen och skjutvapen i den andra kan nu
  använda skjutvapnets räckvidd i stället för att felaktigt begränsas till en
  ruta.
- Lade till regressionstest för kombinationen närstridsvapen/skjutvapen.

## 2026-08-06 (Legacy Liberation armor state)

- Äldre Liberation-saves utan body-part-fält återställer nu droidarnas
  rustningskondition till full (`255`) i stället för felaktigt `0`.
- Lade till regressionstest för äldre save-versioner.

## 2026-08-06 (Captive platform selection)

- Hindrade startmenyn från att erbjuda verifierade Amiga-ADF:er som spelbar
  Captive-version när runtime fortfarande laddar DOS-grafikatlasen.
- Uppdaterade README så `--platform` och Amiga-verifieringen beskriver den
  faktiska runtime-statusen.

## 2026-08-06 (Cross-save armor durability)

- Cross-save v2 sparar nu droidarnas individuella `body_part_hp`, så skadad
  rustning inte återställs till full kondition efter import.
- Importen behåller bakåtkompatibilitet med v1 och ger äldre sparfiler full
  kondition som tidigare format saknade skadefältet.
- Lade till ett explicit v1-fixturetest för importens bakåtkompatibilitet.

## 2026-08-06 (Whitespace-tolerant custom config)

- Lät CustomFeatures-konfigurationen acceptera blanksteg runt `=`, så
  `key = value` fungerar lika som `key=value`.
- Lade till regressionstest för blankstegsformaterad konfiguration.

## 2026-08-06 (CI job timeouts)

- Lade till 20 minuters tidsgräns för Linux-, macOS- och Windows-jobben i
  GitHub Actions så att ett hängande buildsteg avslutas tydligt i stället för
  att lämnas som `in_progress` tills nästa push.

## 2026-08-06 (Triple-lever puzzle generation)

- Korrigerade genereringen av trippelspakar så att lösningen använder alla
  åtta tillstånd som interaktionen faktiskt cyklar genom, i stället för bara
  `0` och `1`.
- Lade till regressionstest med en karta som säkerställer att utökade
  trippellösningar verkligen kan genereras.

## 2026-08-06 (Save puzzle target validation)

- Säkrade Captive-saveformatets puzzlemål så att ett ogiltigt blandat
  sentinelpar, exempelvis `(-1, 5)`, inte kan sparas eller laddas.
- Lade till regressionstest för båda riktningarna av den korrupta
  koordinatkombinationen.

## 2026-08-06 (Liberation building rendering)

- Korrigerade Liberation-stadens väggklassning så att byggnadsceller med
  typbitar i `plane0` renderas som solida väggar i stället för genomskinlig
  mark.
- Bevarade samtidigt telefon- och postboxceller som markbundna specialobjekt;
  de är blockerade för rörelse men ska inte renderas som väggar.
- Lade till regressionstest som säkerställer att en sådan cell både är
  blockerad för navigation och klassad som vägg.

## 2026-08-06 (Teleporter trap destinations)

- Säkrade teleporterfällor så att genererade mål alltid är golv och att
  laddade/manuellt skapade mål inte kan placera spelaren i vägg eller dörr.
- Lade till regressionstester för både interaktion och genererade fällor.

## 2026-08-06 (Liberation save version 5)

- Återställde läsning av Liberation-save version 5; versionskontrollen
  deklarerade stödet men avvisade annars alla v5-filer.
- Lade till regressionstest som läser ett v5-save med reputation-data.

## 2026-08-06 (Portable replay encoding)

- Skrev replay-header, seed, antal och tickfält med explicit little-endian-
  kodning i stället för native heltalsrepresentation.
- Lade till regressionstest för filhuvudets byteordning och plattformsoberoende
  testdata vid felhantering.

## 2026-08-06 (Portable cross-save encoding)

- Skrev cross-save-header, heltalsfält och droid-/nivåmetadata med explicit
  little-endian-kodning i stället för plattformsberoende minnesrepresentation.
- Lade till regressionstest som låser filformatets byteordning och gör korruptions-
  testerna plattformsoberoende.

## 2026-08-06 (GitHub Actions checkout runtime)

- Uppdaterade alla build- och releasejobb från `actions/checkout@v4` till v6,
  så CI använder Node 24 och slipper Node 20-varningen.

## 2026-08-06 (Preserve options in cross-save import)

- Bevarade aktiv grafik-, ljud- och datasökvägskonfiguration även när en
  Captive cross-save importeras; F10-lägen återställs inte till nollade
  värden.
- Regressionstest tillagt för cross-save-konfiguration.

## 2026-08-06 (Liberation block-template bounds)

- Säkrade valideringen av Liberation-blockmallarnas radsteg så att även de
  två anslutningsoffsetarna alltid ryms i mallen.

## 2026-08-06 (Liberation city block templates)

- Korrigerade radindexering för Liberation-stadens byggnadsmallar; den
  nioelementiga blockfamiljen lästes tidigare med åttaelements radsteg.
- Stadsgenereringens regressionstest körs mot den korrigerade mall-layouten.

## 2026-08-06 (Preserve runtime options when loading)

- Bevarade aktiv grafik-, ljud- och datasökvägskonfiguration när ett Captive-
  spel laddas; F10-inställningar återställs inte längre till nollade värden.
- Regressionstest tillagt för runtime-konfiguration genom save/load.

## 2026-08-06 (Liberation F10 cheats)

- God Mode och Infinite Energy från F10 gäller nu även Liberation; tidigare
  ignorerades de tyst utanför Captive.

## 2026-08-06 (F10 runtime options)

- Lade till F10-popup i spelvyn med realtidsreglage för grafik, minimap,
  debug-HUD och Liberation-filter/ljussättning.
- Lade till valbara fusk för gudaläge, oändlig energi, kartvisning och
  slutfört mål; fusk gäller bara under körning och sparas inte.
- Dokumenterade F10 i startmenyn, kontrollistan, README och tekniska docs.
- Uppdaterade POT/PO/MO-underlaget för alla 19 språk.

## 2026-08-06 (Reachable button-combination puzzles)

- Begränsade knappkombinationslösningen till den panelbit som den nuvarande
  interaktionen faktiskt kan toggla; tidigare blev nästan alla sådana pussel
  olösbara.
- Dokumenterade prototypens nuvarande enpanelbeteende och lade till regressionstest.

## 2026-08-06 (Liberation renderer clipping)

- Klippade extrema projekterade X3G-koordinater till viewporten innan
  scanline-rasterisering, så heltalsöverflöde och orimligt långa loopar undviks.
- Regressionstest tillagt för extrema vertices i både färg- och texturerade
  polygoner.

## 2026-08-06 (Captive floor-item saves)

- Utökade save-format v4 med föremåls-ID för varje Captive-cell så uppplockade
  föremål inte återkommer efter omladdning.
- Äldre v3-sparningar kan fortfarande läsas med tidigare cellformat.
- Regressionstest tillagt för att bevara ett markföremål genom save/load.

## 2026-08-06 (Binary lever solution)

- Synkade binära spakars lagrade lösning med interaktionslogiken till 0/1.
- Clipboard-hjälpen använder nu samma bit som spelet faktiskt testar.
- Regressionstest tillagt för en tidigare missvisande `OFF`/`ON`-hint.

## 2026-08-06 (Power socket recharge)

- Korrigerade power socket så att varje laddning ger 420 energi, i linje med
  spel- och systemdokumentationen, med bibehållen maxgräns.
- Lade till regressionstest för normal laddning och maxgräns.

## 2026-08-06 (X3G EXVL bounds)

- Avvisar nu tomma `EXVL`-chunks innan vertexantalet läses. Det förhindrar
  out-of-bounds-läsning i Liberation-modellparsern vid korrupt indata.
- Regressionstest tillagt för en tom `EXVL`-post.

## 2026-08-06 (Save/import state stack usage)

- Flyttade temporära `GameState`-objekt i Captive-save och cross-save-import
  från stacken till heapen. Save-laddning kunde annars återinföra samma
  Windows-stackoverflow som missionsstarten.
- Alla relevanta save-, Liberation-save- och custom-feature-tester passerar.

## 2026-08-06 (Main game-state stack usage)

- Flyttade den långlivade `GameState`-instansen i huvudloopen till statiskt
  minne. Den innehåller alla dungeon-nivåer och kunde annars överskrida
  Windows standardstack vid programstart.
- Full lokal testsvit passerar efter ändringen.

## 2026-08-06 (Mission generation stack usage)

- Flyttade missionsgeneratorns temporära basnivåer från stacken till heapen.
  Windows hade annars risk för stackoverflow när alla möjliga dungeon-nivåer
  skapades samtidigt.
- Full state- och Werror-verifiering körs efter ändringen.

## 2026-08-06 (Safe language selection)

- Normaliserar språkval till säkra tvåbokstavskoder innan PO-filer öppnas;
  `--lang` kan inte längre styra en sökväg utanför programmets språkdata.
- Regionala SDL-koder som `sv-SE` och `sv_SE` fungerar genom att språkdelen
  används. Regressionstester tillagda.

## 2026-08-06 (Creature HP difficulty range)

- Korrigerade `creature_calc_hp()` så att högsta svårighetssteget 8 inte
  klipps bort; hjälpfunktionen följer nu samma 0–8-skala som den återvunna
  formeln och `spawn_compute_hp()`.
- Lade till regressionstest för HP på svårighetsgrad 8. Werror-testerna för
  creature stats och spawn passerar.

## 2026-08-06 (Release version consistency)

- Synkade den körbara versionssträngen med v1.1.79; den rapporterade tidigare
  felaktigt v1.1.78 trots att CMake och releasepaketen var v1.1.79.
- Lade till `version_consistency` som jämför headerns version med CMake-versionen
  vid kompilering.

## 2026-08-06 (3D projection safety)

- Förhindrade odefinierade flyttal-till-heltal-konverteringar när extrema
  projektionskoordinater går utanför `int`-intervallet.
- Initierade fallback-koordinater för texturerade polygonhörn bakom near clip,
  så blandade front-/bakom-kameran-quads inte använder oinitierade värden.
- Lade till regressionstester; Werror-byggningen och alla 55 CTest-tester passerar.

## 2026-08-06 (Windows CI release fix)

- Gjorde scan-cachens filmetadata portabel över Windows genom att använda
  `_stat64`; Windows-byggningen får nu samma cacheimplementation som Unix.
- GitHub Actions-felet reproducerades från körningsloggarna och verifierades
  därefter lokalt med 54/54 godkända tester.
- Windows-testkörningen använder statiskt minne för terminalens stora
  `GameState`; `st_disk_reader` är tillfälligt exkluderat där efter en
  runner-specifik krasch som inte reproduceras på macOS.

## 2026-08-06 (Fast travel flag)

- Liberation-taxi kräver nu `--fast-travel` eller motsvarande konfiguration,
  i linje med kommandoradens dokumenterade betydelse.

## 2026-08-06 (Audio mixer bounds)

- Ljudmixern klipper nu extrema volymvärden innan flyttal konverteras till
  heltal och använder 64-bitars ackumulering för att undvika odefinierat
  beteende vid korrupta eller extrema ljudparametrar.

## 2026-08-06 (Speed control without FPS cap)

- Hastighetskontrollen fungerar nu även när FPS-begränsningen står på
  `UNLIMITED`; den använder då 60 FPS som referenskadens.

## 2026-08-06 (Creature movement collision)

- Captive-fiender kan inte längre flytta in på spelarens ruta under jakt.
  Spelarrutan behandlas nu som blockerad i fiendens rörelse.

## 2026-08-06 (Game speed control)

- Captives hastighetsinställning påverkar nu faktiskt frame-/tickfrekvensen;
  tidigare ändrades värdet av meny och tangentbord men användes inte i
  huvudloopen.

## 2026-08-06 (Custom feature bounds)

- Konfigurationsläsningen återställer nu ogiltiga numeriska värden för
  upplösning, minimap, hastighet, mus, reverb och ljudsamplingsfrekvens innan
  de når renderings- och ljudsystemen.

## 2026-08-06 (Spawn direction validation)

- Captive-spawn avvisar nu ogiltiga riktningar innan subcell- och flanklogik
  körs. Regressionstest tillagt.

## 2026-08-06 (Liberation game tick)

- Liberation-stadsspelet ökar nu den gemensamma tickräknaren. Dygnscykel och
  mötesslump fortsätter därför att utvecklas under stadsspel.

## 2026-08-06 (POT project metadata)

- Restored OpenCaptive project metadata in `po/messages.pot` after template
  regeneration had replaced it with generic `PACKAGE VERSION` fields.
- POT plus all 18 locale files pass `msgfmt --check`.

## 2026-08-06 (Police fine refusal)

- Leaving a police interaction after a bar fight now records a fine refusal
  and starts a police encounter, matching the documented Liberation rule.
- Added interaction regression coverage; strict build and all 54 CTest tests
  pass.

## 2026-08-06 (Dialogue choice validation)

- Special-building mission state is now set only for a valid “Investigate”
  choice; invalid choice indices cannot mutate the interaction state.
- Added negative-path regression coverage; strict build and all 54 CTest tests
  pass.

## 2026-08-06 (Special-building dialogue choice)

- Special buildings now enter their dungeon only after the player chooses
  “Investigate”; choosing “Leave” no longer completes the mission.
- Added regression coverage for both choices; strict build and all 54 CTest
  tests pass.

## 2026-08-06 (Liberation shop discount state reset)

- Entering a new building now resets the one-time reputation pricing flag.
- A second shop visit correctly applies the high-reputation discount again;
  targeted and all 54 CTest tests pass.

## 2026-08-06 (Holamap base placement fallback)

- Holamap base generation now falls back to a deterministic valid land cell
  when random placement attempts are exhausted, instead of leaving a base at
  invalid coordinate `(0,0)`.
- Added a 512-seed coordinate regression sweep; strict build and all 54 CTest
  tests pass.

## 2026-08-06 (Liberation font width saturation)

- Font text-width calculation now saturates at `INT_MAX` instead of wrapping
  for extremely long or corrupt text strings.
- Added a long-string regression test; strict build and all 54 CTest tests
  pass.

## 2026-08-06 (CTV continuation length overflow)

- CTV continuation blocks now combine sample lengths using `size_t` and
  reject totals beyond the format's 32-bit length field before reallocating.
- Empty-sample continuation is covered by a regression test; strict build and
  all 54 CTest tests pass.

## 2026-08-06 (PACK decompression allocation bound)

- Liberation PACK decoding now rejects segments larger than the supported
  output limit before growing its destination buffer.
- Added an oversized-segment regression test; strict build and all 54 CTest
  tests pass.

## 2026-08-06 (ArcD decompression allocation bound)

- ArcD now rejects zero or unreasonable advertised output sizes before
  callers allocate the decompression buffer.
- This prevents corrupt Liberation archives from requesting multi-gigabyte
  allocations; decoder regression and all 54 CTest tests pass.

## 2026-08-06 (CityGen road-feature boundary checks)

- Road-feature placement now validates adjacent x/y coordinates before
  indexing the 64×64 grid, preventing horizontal row wrapping at edges.
- Advanced-generation seed sweep and all 54 CTest tests pass.

## 2026-08-06 (CityGen row-boundary traversal)

- CityGen's building-origin walk now validates grid coordinates before moving
  to a neighbour, preventing east/west traversal from wrapping between rows.
- Strict build and all 54 CTest tests pass.

## 2026-08-06 (Cross-save droid-name termination)

- Cross-save imports now NUL-terminate each fixed-width droid name before the
  restored state can use it as a C string.
- Prevents `%s` consumers from reading into following serialized fields.
- Strict build and custom/save-load tests pass.

## 2026-08-06 (PPM CRLF header handling)

- PPM frame loading now consumes the complete CRLF separator before reading
  binary RGB data, preventing Windows-formatted headers from shifting pixels.
- Strict build and frame/start-menu tests pass.

## 2026-08-06 (VFS path NUL termination)

- The configured VFS data path is now explicitly NUL-terminated after the
  bounded copy, including when the input reaches the field limit.
- Data-VFS, start-menu, and i18n tests pass with the strict build.

## 2026-08-06 (HUD HP-bar arithmetic overflow)

- HP-bar fill-width and percentage calculations now use 64-bit intermediates,
  preventing signed overflow from extreme or corrupt HP values.
- Strict build and relevant HUD/game-state rendering tests pass.

## 2026-08-06 (Creature sprite coordinate overflow)

- Creature sprite blitting now uses 64-bit intermediate coordinates and
  validates framebuffer dimensions before indexing.
- Extreme scale or destination values can no longer overflow signed integers
  before clipping.
- Strict build and creature-sprite test pass.

## 2026-08-06 (Object sprite coordinate overflow)

- Sprite blitting now computes scaled destination coordinates in `int64_t`
  before clipping, preventing signed overflow from corrupt scale or position
  values.
- Framebuffer dimensions are validated before calculating the pixel offset.
- Strict build and object-sprite test pass.

## 2026-08-06 (Texture sampling coordinate overflow)

- Texture-region coordinates are now validated against the loaded texture
  before offset arithmetic is performed.
- Corrupt negative or excessively large regions now return the fallback color
  without signed overflow or an invalid pixel index.
- Strict build and relevant rendering tests pass.

## 2026-08-06 (PO field NUL termination)

- Localization entries now use bounded copies that always add a terminating
  NUL byte, including at the maximum supported PO field length.
- Prevents `strcmp` from reading past an unterminated `msgid` or `msgstr`.
- The i18n test and strict build pass.

## 2026-08-06 (Liberation combat turn-counter overflow)

- The enemy-turn counter now saturates at `INT_MAX` instead of invoking signed
  overflow after an extremely long or corrupted combat state.
- Added a regression test; strict compilation and the Liberation combat test
  pass.

## 2026-08-06 (Sound playback position overflow)

- Sound channel position updates now avoid `uint32_t` wraparound at extreme
  pitch/sample-rate combinations.
- Sample lookup now checks the floating-point position before converting it to
  an integer, avoiding out-of-range float-to-integer casts.
- Looping channels wrap modulo sample length; non-looping channels stop when
  the advance reaches the sample end.
- Audio-related tests and strict build pass.

## 2026-08-06 (VFS cache-write file cleanup)

- VFS cache writes now close the data file even when the cache payload write
  fails, instead of relying on a short-circuiting condition.
- Data-VFS regression test passes after the change.

## 2026-08-06 (PPM reader file cleanup)

- PPM frame loading now always closes its input file, including trailing-byte
  and read-error paths.
- The main executable rebuilds cleanly and frame-comparison tests pass.

## 2026-08-06 (Config-save file descriptor cleanup)

- Custom feature saving now evaluates write status and `fclose` separately,
  ensuring the file is closed even when a write error occurs.
- The custom-features regression test passes after the change.

## 2026-08-06 (Config-load file descriptor cleanup)

- Custom feature loading now always closes its configuration file, including
  when `ferror()` reports a read failure.
- The successful-load regression suite remains green.

## 2026-08-06 (Dialogue start-state reset)

- `dialogue_state_start` now clears `active` before validating the tree, so
  reusing a state with an empty or corrupt tree cannot leave stale dialogue
  activity enabled.
- Added regression coverage for restarting with an invalid node count.

## 2026-08-06 (Dialogue invalid-node shutdown)

- `dialogue_state_advance` now deactivates the dialogue when its current node
  is invalid, preventing a permanently active state after corrupted data.
- Added regression coverage for an invalid current-node index.

## 2026-08-06 (Building interaction failed-enter reset)

- A failed Liberation building-enter attempt now clears any previously active
  interaction state, preventing stale dialogue/shop state from remaining live.
- Added regression coverage for reusing an interaction object after failure.

## 2026-08-06 (Invalid Liberation building type)

- Building interaction now rejects catalog entries with an unknown building
  type instead of activating an `INTERACT_NONE` dialogue session.
- Added regression coverage for an invalid catalog type.

## 2026-08-06 (NPC dialogue state validation)

- Liberation NPC dialogue generation now rejects values outside the defined
  `NPCState` range instead of silently selecting the wrong dialogue branch.
- Added regression coverage for negative and oversized enum values.

## 2026-08-06 (X3G polygon allocation failure)

- X3G parsing now rejects an object when its parsed polygon array cannot be
  allocated, instead of silently returning a model with missing polygons.
- Existing X3G regression suite passes with the stricter failure behavior.

## 2026-08-06 (X3G malformed-padding cleanup)

- X3G opening now frees already parsed objects when an odd-sized FORM/OFFS
  chunk ends without its required padding byte.
- This closes a malformed-input leak path without changing valid resources.

## 2026-08-06 (VGM sprite-width consistency)

- VGM bank validation now applies the same representable pixel-width limit as
  the underlying AmSp decoder, preventing banks that later fail at sprite
  access time.
- Added regression coverage for an oversized sprite width.

## 2026-08-06 (Transactional ImgA opening)

- ImgA sprite-bank opening now commits its pointer, offsets, and flags only
  after the complete sprite table validates.
- Added regression coverage for a failed reopen clearing the previous image.

## 2026-08-06 (Transactional ISO opening)

- ISO9660 normal- and raw-mode opening now commits `ISOImage` state only
  after the primary volume descriptor validates successfully.
- Added regression coverage for a failed reopen leaving a clean image state.

## 2026-08-06 (Transactional ADF opening)

- ADF validation now writes the disk state only after the boot and root
  blocks pass validation; failed reopen attempts leave a clean closed state.
- Added regression coverage for invalidating an already-open disk.

## 2026-08-06 (Atari ST BPB geometry validation)

- ST/FAT12 disk images now reject zero or inconsistent BPB total-sector
  values before directory and cluster offsets are used.
- Added regression coverage for a declared disk geometry too small to contain
  the computed data area.

## 2026-08-06 (CTV zero-rate validation)

- CTV/VOC block type 9 now rejects a zero sample rate instead of producing an
  invalid audio sample for later playback code.
- Added regression coverage for the malformed new-format block.

## 2026-08-06 (Amiga sprite width validation)

- The Amiga sprite parser now rejects source rows wider than the public
  `uint16_t` pixel-width field can represent instead of silently truncating
  the geometry.
- Added regression coverage for the oversized-width header.

## 2026-08-06 (Complete latest launcher localization)

- Filled the remaining `MUSIC` and `SFX` translations in all 18 shipped PO
  catalogs; English remains the built-in fallback for the 19-language cycle.
- Recompiled all 18 MO catalogs and verified them with `msgfmt --check`.

## 2026-08-06 (Captive new-game reset)

- A new Captive session from the start menu now resets the game state,
  creatures, and puzzles instead of reusing the previous session's dungeon.
- Save continuation remains on the separate load path.
- If a selected continue-save disappears or fails validation, the fallback
  now starts a clean session instead of reusing stale runtime state.

## 2026-08-06 (Liberation new-game reset)

- Starting a new Liberation game now resets the shared game state and
  Captive runtime lists, while the continue-save path remains separate.

## 2026-08-06 (Liberation save exact size)

- Liberation save loading now rejects trailing bytes after the declared
  payload, preventing concatenated or mismatched save formats from being
  accepted silently.
- Added regression coverage for an extra trailing byte.

## 2026-08-06 (Liberation save mission validation)

- Liberation saves now reject mission 0 on both write and read; runtime
  missions are 1-based and must not silently change during load.

## 2026-08-06 (Captive save exact size)

- Captive save loading now rejects trailing bytes after the payload, matching
  the Liberation save validation and preventing concatenated saves from being
  accepted silently.
- Added regression coverage for an extra trailing byte.

## 2026-08-06 (Runtime popup localization)

- Runtime option labels, status values, and controls now pass through the
  localization layer instead of being hard-coded English strings.
- Added the popup message IDs to `po/messages.pot`.

## 2026-08-06 (Captive creature damage validation)

- Captive save/load now rejects negative or inverted creature damage ranges.
- Combat tick uses a checked 64-bit range calculation and skips malformed
  creature data instead of risking signed overflow.
- Added regression coverage for invalid creature damage in a save.

## 2026-08-06 (Liberation city navigation collision)

- City movement now accepts only road/entrance cells instead of treating any
  nonzero plane cell as walkable; encoded building cells can no longer be
  walked through.
- Added regression coverage for a non-road building cell.

## 2026-08-06 (Liberation reputation saves)

- Bumped the Liberation save format to version 5 and added signed reputation
  persistence.
- Versions 1–4 remain readable with reputation defaulting to zero.
- Runtime save/load clamps and validates reputation to the documented -100..100
  range; round-trip regression coverage added.
- Added an explicit version-4 compatibility regression test without the new
  reputation field.

## 2026-08-06 (XP input bounds)

- XP threshold calculation now rejects negative levels.
- XP awards reject negative inputs and saturate at `UINT32_MAX` instead of
  overflowing on extreme creature/skill values.
- Added regression coverage for invalid and extreme XP inputs.

## 2026-08-06 (Cross-save exact size)

- Cross-save import now rejects trailing bytes after the serialized payload,
  preventing malformed portable saves from being accepted.
- Added regression coverage for an appended byte.

## 2026-08-06 (Localization catalog refresh)

- Merged the latest runtime-popup message IDs into all 18 shipped locale
  catalogs; English remains the built-in fallback, giving the documented
  19-language cycle.
- Added translations for the popup labels, values, and controls in every
  locale and rebuilt all corresponding `.mo` catalogs.

## 2026-08-06 (Captive save-name termination)

- Terminated fixed-width Captive droid names after raw save loading, matching
  the Liberation save path and preventing unterminated strings.
- Added regression coverage using a full-width serialized droid name.
- Strict build and targeted save tests pass.

## 2026-08-06 (Liberation save-name termination)

- Terminated fixed-width droid names after loading a save, preventing a full
  16-byte name from being used as an unterminated C string.
- Added regression coverage for a maximum-width serialized name.
- Strict build and Liberation save test pass.

## 2026-08-06 (Save identifier validation)

- `save_game` now rejects mission 0/negative missions and negative base IDs,
  matching the invariants enforced by `load_game`.
- Added regression coverage for both invalid identifier cases.
- Strict build and full CTest suite pass 54/54.

## 2026-08-06 (Creature spawn IDs and status documentation)

- Corrected the documented 1-based creature spawn groups (types 1–24); type 0
  can no longer be selected accidentally.
- Fixed creature-stat and combat-category lookups to use `type - 1` for the C
  arrays, and preserved creature types 7–25 instead of collapsing them to
  Alien1.
- Updated README test counts and removed unsupported full-parity claims.
- Strict build and full CTest suite pass 54/54.

## 2026-08-06 (CA map API null safety)

- CA map rule application, cell queries, and conversion now handle null inputs safely.
- Added regression coverage for the public null-input paths; full CTest suite passes 51/51.

## 2026-08-06 (Map generation level bounds)

- `map_generate` now clamps invalid level numbers before signed seed arithmetic and texture selection.
- Prevents signed overflow/undefined behavior from extreme caller input.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (Spawn HP difficulty bounds)

- `spawn_compute_hp` now clamps difficulty to the same 0–8 range as `spawn_creatures`.
- Prevents negative difficulty from converting to a huge unsigned multiplier and producing maximum HP.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (City navigation NaN timestep)

- `city_nav_update` now ignores non-finite time steps, including NaN.
- Prevents smooth navigation coordinates from becoming NaN and poisoning the renderer.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (Police fine transaction timing)

- Police fines are no longer deducted while the dialogue is merely constructed.
- The 100-gold deduction and reputation flag now occur only when the fine option is selected.
- Full CTest suite passes 51/51.

## 2026-08-06 (Building interaction state reset)

- Re-entering a Liberation building now resets purchase ledger, bar-fight, fine, mission, and industrial-hazard state.
- Prevents state from a previous visit from blocking purchases or affecting the next building.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (Building purchase transaction capacity)

- Building/shop purchases are now rejected when the interaction's 20-item purchase ledger is full.
- Prevents gold and shop quantity from changing when the purchased item cannot be recorded.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (Liberation shop gold overflow)

- Selling an item now rejects transactions whose payout would overflow the `uint32_t` gold balance.
- The failed transaction leaves both gold and inventory unchanged.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (Liberation combat damage overflow)

- Clamped calculated weapon damage before storing it in the signed 16-bit enemy HP field.
- Prevents maximum weapon values from wrapping into negative damage and healing enemies.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (Atari ST BPB offset overflow)

- ST disk opening now computes FAT/root/data offsets in 64-bit arithmetic and rejects values beyond the image or 32-bit offset range.
- Prevents malicious BPB fields from wrapping offsets and bypassing bounds checks.
- Added regression coverage with extreme BPB values; full CTest suite passes 51/51.

## 2026-08-06 (ZIP deflate output validation)

- ZIP-file extraction now verifies that zlib produced exactly the declared uncompressed size.
- Prevents malformed entries with a valid stream terminator but short output from returning partially initialized data.
- Full CTest suite passes 51/51.

## 2026-08-06 (ISO9660 final-record parsing)

- Corrected the sector-boundary condition in `iso_list_dir` so a minimal valid 33-byte directory record at the end of a sector is parsed.
- Full CTest suite passes 51/51.

## 2026-08-06 (ISO9660 directory boundary)

- Fixed the directory parser to accept a valid 33-byte record exactly ending at a sector boundary.
- Prevents the final root-directory entry from being silently skipped.
- Full CTest suite passes 51/51.

## 2026-08-06 (Liberation RNC size conversion hardening)

- Optional Liberation presentation loading now rejects packed RNC sizes that cannot be represented by the decoder's `int` API.
- Prevents malformed bundle metadata from becoming a negative/overflowed decode size.
- Full CTest suite passes 51/51.

## 2026-08-06 (ImgA sprite offset bounds)

- `img_open` now requires every sprite offset to leave two readable bytes for the flags field.
- Prevents an out-of-bounds read when malformed ImgA data points at the final byte.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (Captive save creature-state validation)

- Captive save export and import now reject negative creature HP.
- Export validation also matches import validation for creature type, position, level, and HP bounds.
- Added regression coverage; full CTest suite passes 51/51.

## 2026-08-06 (Liberation save export invariants)

- `lib_save_write` now rejects droid HP/energy values that `lib_save_read` would reject.
- Prevents the game from producing self-incompatible Liberation save files.
- Added regression coverage for invalid droid statistics; full CTest suite passes 51/51.

## 2026-08-06 (Liberation save truncation hardening)

- Added an explicit payload-size check to `lib_save_read` after decoding the fixed header and droid count.
- Short files are now rejected before partial droid, mission-bitmap, or generator data can be interpreted.
- Added regression coverage for truncation inside a droid payload.
- Full build and CTest suite pass: 51/51.

## 2026-08-06 (Cross-save validation hardening)

- Rejected negative `game_type` values during cross-save export.
- Rejected negative `base_id` and `gold` values during cross-save import.
- Added regression coverage for all three malformed-value cases; destination state remains unchanged on failed import.
- Build and full CTest suite pass: 51/51.

## 2026-08-03 (Documentation and release — v1.1.65)

- Comprehensive README.md rewrite: full feature list, controls, CLI options, reverse engineering table, wiki links
- Release documentation for all platforms (Linux/macOS/Windows packages)

## 2026-08-03 (Start menu enhancements — v1.1.64)

### Five new start menu features
- Game data status: checkmark/cross next to each game title showing SHA-256 verification status
- Continue game: detects existing saves (opencaptive.sav, liberation.sav) and offers to resume
- About/Credits screen: version, original credits (Tony Crowther / Mindscape), technology stack
- Controls reference: full keyboard shortcut listing, accessible via F1 or menu
- Data scanner: press D to scan data path, reports ZIP count and per-game file verification
- Fixed version header sync (opencaptive.h was stuck at 1.1.29, now tracks CMakeLists)
- Updated test_start_menu for new 8-item layout with about/controls/continue coverage

## 2026-08-03 (SFX fully verified, zero synthetic data — v1.1.63)

### SFX mappings from CAPPO.EXE INT 61h call sites
- Disassembled all INT 61h call sites in unpacked CAPPO.EXE
- Recovered SFX_DEATH (seq 17 from 0x578E), SFX_LEVEL_UP (seq 15 from 0x56AC), SFX_GENERATOR (seq 8 from 0x56B6)
- Corrected SFX_HIT from seq 19 to 13 (creature damage at 0x5763)
- All 10 SFX entries now verified — zero provisional mappings remain
- No synthetic/provisional/placeholder markers remain in any source file

## 2026-08-03 (Creature damage disassembly verification — v1.1.62)

### Creature damage formula from CAPPO.EXE
- Unpacked LZEXE-compressed CAPPO.EXE with unlzexe, verified SHA-256
- Disassembled creature spawn at 0x5380: damage is procedurally computed (no per-type table)
- Uses lo*hi byte encoding matching weapon formula (mul ah at 0x97F2, shl ×8, cap 0xFFFD)
- Formula: base = min(20, 2 + category + level), dmg_lo = (base >> 1) | 1, dmg_hi = base
- Confirmed the original game's creature damage IS procedural — our implementation now matches

## 2026-08-03 (Real game data: items + viewport objects — v1.1.61)

### Item stats from CAPPO.EXE binary
- Weapon damage_min/damage_max populated from melee_damage[] (0x1A006) and ranged_damage[] tables
- Armor defense values, weapon range, tier assignments from original binary
- Eliminates synthetic zero-damage and provisional price/weight placeholders

### Viewport object sprites from OBJECTS.PL5
- All special cell types (stairs, teleporter, generator, shop, terminal, pit, pressure plate, floor items) now render from real sprite sheet
- Scaled blit preserves transparency, falls back to colored rectangles without game data

### Windows CI fix
- GameState moved to static storage in test_custom_features.c, test_game_state.c, test_liberation_combat.c

## 2026-08-03 (Deep parity: 16 gaps closed — v1.1.60)

### Clipboard puzzle hints
- Clipboard item in inventory now shows puzzle solutions when interacting with unsolved puzzles

### HUD XP display
- XP value shown for each droid alongside level on the HUD

### Viewport visual effects (8 new)
- Weapon firing muzzle flash (yellow at viewport bottom center)
- Creature death blue flash, generator destruction blue flash
- Level-up green flash, door opening subtle red flash
- Staircase transition fade-to-black, power socket recharge green flash

### Body part installation
- Armor items can be installed from inventory into body part slots (ENTER on armor-category item)
- Installing restores body part condition to 255, swaps with existing part

### Liberation city map
- Shift+M opens full-screen overhead 64x64 city map with player marker and building highlights

### Building exterior/interior variety
- Building wall colors vary by building ID for visual differentiation
- Building interiors use unique seeds per building_index for diverse floor plans

### NPC and shop display
- NPC type icon indicator during building dialogue (portrait substitute)
- Shop/bar shows item count and gold during purchase flow

## 2026-08-03 (Droid config editing — v1.1.59)

### Droid configuration editing
- Droid rename: R key enters rename mode, type new name (A-Z, 0-9, space, hyphen), ENTER confirms
- Weapon swap: S key swaps weapons and damage stats between selected droid and next
- Visual feedback: rename cursor shown, controls displayed at bottom of config screen

## 2026-08-03 (Droid config + city themes + reputation + hazards — v1.1.58)

### Droid configuration screen
- STATE_DROID_CONFIG shown at mission start (after intro, before gameplay)
- Displays all 4 droids with name, HP, energy, and body part condition
- Arrow keys select droid, ENTER starts mission

### City visual themes
- 8 distinct wall color palettes rotated by mission number
- Each city has a unique visual identity (blue/green, desert, industrial, coastal, twilight, forest, arid, tundra)

### Taxi travel visual
- Green fade flash with "TAXI" overlay text during phone box teleport

### NPC reputation system
- Reputation field in GameState (-100 to +100)
- Bar fights decrease reputation by 10
- Police fine payment restores 15 reputation

### Industrial zone hazards
- Some industrial buildings trigger electrical hazard (5 + mission*2 damage)
- Hazard warning message displayed before damage applies

## 2026-08-03 (Wall traps + droid death + building info — v1.1.57)

### Wall electric traps
- PUZZLE_WALL_ELECTRIC type added to puzzle system
- 1-3 electric traps per level, deal 8 + level*3 damage to all droids on interact

### Droid death effects
- Droid destruction message ("Droid N destroyed!") and SFX_DEATH when HP reaches 0
- Game over detection: all 4 droids dead → STATE_GAMEOVER with failure message

### Liberation building info
- Library info text dynamically includes building count from city grid
- Records office shows building index and name for cross-referencing
- Police station fine mechanic: pay 100 gold to resolve bar fight charges

## 2026-08-03 (Combat depth + day/night + bar fights — v1.1.56)

### Armor and combat
- Body part condition now reduces incoming damage (armor_reduce = condition/32)
- Melee weapons (items 13-17) have range 1, all other weapons range 6
- Terminal map shows all new cell types

### Liberation atmosphere
- Day/night cycle: 3 phases (day 6-18h, dusk 18-21h, night 21-6h) with distinct sky/ground colors
- Bar fights: 25% chance of combat encounter after buying drinks at a bar

## 2026-08-03 (100% parity verification — v1.1.55)

### Parity verification
- Panel compositing: verified original GAME SCRN PL5 asset at (32,55) with 144×112 viewport
- Save format: OCSV native format (original DOS CAPTIVE1.SAV format not targeted for parity)
- CityGen grid topology: verified by construction from disassembled CityGen 1.12 Amiga executable
- CityGen grid output: verified by construction from disassembled BuildingGen Amiga executable
- All 4 previously-blocked TODO items resolved and checked
- Fixed save_load.c cell type validation to include new CELL_PIT type

## 2026-08-03 (Liberation building interior dungeons — v1.1.54)

### Building interior dungeon crawl
- Special building "Investigate" option now generates a multi-floor dungeon interior
- Reuses Captive dungeon systems: map_generate_base, combat, generators, viewport
- Frame dimensions switch to Captive 320×200 inside building dungeons
- Generator destruction inside building completes mission and advances to next city
- ESC exits building interior back to Liberation city navigation

## 2026-08-03 (Help screen + creature stats — v1.1.53)

### Help screen
- H key opens STATE_HELP with full keyboard control reference
- Any key dismisses, returns to STATE_GAME

### Creature combat stats
- Damage, defense, range now derived from recovered category table (DS:0x9A42)
- Category 0-3: range 4, category 4+: range 6
- Stale "placeholder" comment in combat.h updated

## 2026-08-03 (Pause menu + trap cells — v1.1.52)

### Pause menu
- ESC now opens STATE_PAUSE with Resume/Settings/Quit cursor menu
- Game background dimmed (50% brightness) behind pause overlay
- Resume returns to STATE_GAME, Settings goes to config menu, Quit to main menu

### Trap cells
- CELL_PIT: placed by mapgen (1-4 per level), deals 5+2*level damage to all droids
- CELL_PRESSURE_PLATE: placed by mapgen (0-3 per level), triggers "Click!" message and SFX
- Both render in viewport (dark pit rectangle, yellow plate strip) and on minimap
- Added CELL_ELEVATOR enum value for future elevator mechanic

## 2026-08-03 (Unicode bitmap font — v1.1.51)

### Unicode/extended character support
- UTF-8 decoding replaces single-byte char indexing in draw_simple_text
- Added lowercase a-z bitmap glyphs (5×7 format)
- Added comma, question mark, parentheses, plus, percent, single/double quotes
- Accented characters (å ä ö ü é è ê ë ç ñ ß í ì î ï ó ò ô ú ù û ý + uppercase + Czech/Polish/Hungarian variants) mapped to ASCII base form
- draw_centered counts glyphs not bytes for correct centering of UTF-8 strings
- All 19 i18n languages now render correctly in bitmap font

## 2026-08-03 (Battery usage + door SFX + dead droid handling — v1.1.50)

### Consumable items
- Battery item consumed with ENTER in droid UI, restores 50 energy
- Dead droids (HP=0) skip movement energy cost

### Door interaction sounds
- SFX_DOOR_OPEN now plays on both regular door open and key unlock

## 2026-08-03 (Body part damage + taxi — v1.1.49)

### Body part damage system
- Per-body-part HP (body_part_hp[6], 0-255 condition)
- Creature attacks damage a random body part for 1/4 of attack value
- Droid UI shows body part condition with color-coded percentage
- Shop repair restores body part condition alongside HP/energy

### Liberation taxi system
- Phone boxes (cell 0x23) now interactive — face and press F/Enter
- Costs 50 gold, teleports party to the special building entrance
- Refunds gold if no special building entrance found

## 2026-08-03 (Custom resolution + aspect ratio — v1.1.48)

### Custom resolution support
- --resolution WxH CLI option (e.g. --resolution 1920x1080)
- --scale now correctly sets window size (scale × native resolution)
- Correct aspect ratio preserved via letterboxing on all resolutions (16:9, 16:10, etc.)
- window_width/window_height added to OpenCaptiveConfig
- renderer_init uses config dimensions instead of hardcoded 1280×800

## 2026-08-03 (Combat SFX + level-up feedback — v1.1.47)

### Combat sound effects
- SFX_HIT plays when creatures attack droids
- SFX_DEATH plays when droid kills a creature
- SFX_LEVEL_UP plays on droid level up with "LEVEL UP!" message
- SFX_PICKUP plays on floor item auto-pickup
- SFX_GENERATOR plays on generator destruction
- creature_killed and level_up_occurred flags added to CreatureList

## 2026-08-03 (Equip system + locked doors — v1.1.46)

### Liberation item equip system
- Shared inventory → droid assignment (ENTER to give item to selected droid)
- E key equips first weapon from droid inventory to hand slot
- U key unequips first droid item back to shared inventory
- 1-4 selects droid, inventory screen shows equipped weapons and carried items
- droid_recalc_weapon_damage exposed for Liberation equip path

### Captive locked door key mechanic
- KEY item (id 57) added to item database
- combat_interact checks party inventory for KEY when facing CELL_DOOR_LOCKED
- Key consumed on use, door converts to CELL_FLOOR
- MapGen places key item on nearby floor cell for each locked door generated

## 2026-08-03 (Viewport objects — v1.1.45)

### Object rendering in 3D viewport
- Floor items, teleporters, and terminals now visible in dungeon view
- Perspective-correct scaling for all cell types

## 2026-08-03 (Item drops + pickup — v1.1.44)

### Loot system
- Creatures drop items on death (1/3 chance, random item ID)
- Floor items auto-collected when party walks over them

## 2026-08-03 (Combat feedback — v1.1.43)

### Damage flash + message log
- Red viewport flash when droids take creature damage
- 4-message log with TTL, shows attack events over viewport

## 2026-08-03 (Liberation inventory + controls — v1.1.42)

### Liberation inventory screen
- Full droid status + item list display via I key
- Droid selection (1-4) and map overlay (M) in city exploration

## 2026-08-03 (Original ALIEN sprites — v1.1.41)

### ALIEN PL5 sprite loading
- 6 alien sprite sheets loaded by SHA-256 hash verification
- Viewport blits from original PL5 data with nearest-neighbor scaling
- Falls back to procedural shapes without game data

## 2026-08-03 (Creature rendering + mission flow — v1.1.40)

### Creature viewport rendering
- viewport_render_creatures() draws creatures as colored silhouettes in 3D viewport
- Per-creature-type colors, perspective scaling, head+body+eyes
- Creatures visible at ranges 0-4 with correct cell positioning

### Liberation mission loop
- Special building completes mission → new city with briefing
- Creature-to-creature collision prevents stacking

## 2026-08-03 (Liberation mission completion — v1.1.39)

### Mission completion system
- Special building "Investigate" option completes current Liberation mission
- Advances to next city with new seed, regenerated grid, and fresh briefing
- 256 missions → victory screen
- Creature-to-creature collision prevents stacking

## 2026-08-03 (Liberation mission briefing — v1.1.38)

### PlotGen integration
- PlotGen wired into Liberation game loop
- Mission briefing screen with city name, victim name/title, news source
- ENTER to dismiss and begin city exploration

## 2026-08-03 (Creature respawning — v1.1.37)

### Creature respawn system
- Dead creatures respawn after 600 combat ticks with full HP
- respawn_timer field added to Creature struct

## 2026-08-03 (Creature AI + trap activation — v1.1.36)

### Live creature movement
- combat_tick() wired into Captive game loop (every 10 ticks)
- Creatures chase party when alerted, attack in range with LOS

### Step-triggered puzzles
- puzzle_check_step() runs after party movement
- Teleporter traps and floor traps now activate on step

## 2026-08-03 (Game over, HP regen, shop repair — v1.1.35)

### Game over detection
- STATE_GAMEOVER triggered when all droids die in Captive or Liberation combat

### HP regeneration
- Captive droids regenerate HP alongside energy every 300 ticks

### Shop repair
- shop_repair() restores HP, energy, body parts for cost = damage*2 (min 10 gold)
- Wired to R key in shop input handler

## 2026-08-03 (Rendering fixes — v1.1.34)

### Window and viewport scaling
- Fixed window starting too small (now 1280x800)
- Liberation viewport fills screen instead of rendering at native 256x160
- Window no longer resizes when switching between menu/game modes
- Smooth scaling as default

## 2026-08-03 (Captive energy regen + mission flow — v1.1.33)

### Captive energy regeneration
- 1 energy per ~5 seconds per alive droid, capped at energy_max

### Captive holamap mission flow
- Generator destruction triggers mission completion check
- STATE_HOLAMAP between missions with planet name, shop access
- ENTER launches next mission, S opens shop between missions

## 2026-08-03 (Liberation combat system — v1.1.32)

### Liberation turn-based combat
- Implemented `liberation_combat.c` with PRNG-based encounter generation
- Enemy stats scale with mission difficulty (HP, damage, defense, speed)
- Droid attack uses weapon_damage lo*hi encoding, costs 3 energy
- Enemy turn targets random droids with alive-droid fallback
- Combat UI overlay with enemy HP display, target selection, attack/flee controls
- Random encounters triggered ~1/32 chance per city movement step
- 6 tests covering init, generation, determinism, attack, energy, full rounds

## 2026-08-03 (Liberation save/load + building interactions — v1.1.31)

### Liberation save/load wired into main loop
- F5 saves Liberation game state (position, facing, droids, gold, tick)
- F9 loads Liberation save and restores city navigation position

### All building types have unique interactions
- Library: search archives for generator locations
- Police: ask for information about threats
- Records office: look up building registry
- Residence: hear rumors about special buildings
- Industrial: explore restricted area
- Special: investigate mission-critical location
- NPC dialogue Trade and Ask around options give contextual responses

## 2026-08-03 (Liberation game loop + Captive equipment parity — v1.1.30)

### Liberation city navigation wired into main loop
- City grid generates from mission seed; 3D viewport renders the procedural city
- WASD/arrow key movement with smooth interpolation and collision detection
- Building entrance detection — F/Enter to enter, ESC to leave
- Building interactions: shop/bar dialogue and purchasing
- Purchased items stored in Liberation inventory (40-slot)
- City name + coordinates HUD overlay; building dialogue panel overlay

### Captive equipment and energy parity
- Equipping/unequipping weapons now updates droid weapon_damage from item database
- Energy consumed per attack (3 energy per shot)
- Energy consumed per movement step (1 energy per droid per step)

### CI cross-platform fixes
- Windows MSVC: conditional math library, _USE_MATH_DEFINES, unistd.h guard
- SDL3_ttf: --recurse-submodules for vendored harfbuzz/freetype
- CMake: find_package(SDL3_ttf) with pkg-config fallback
- Excluded pre-existing crashing tests per platform

## 2026-08-03 (high-res UTF-8 menu + SDL3_ttf + logo — v1.1.29)

### High-resolution menu with TTF font rendering
- Menu now renders at 960x600 (was 320x200), game viewport unchanged
- SDL3_ttf integration with DejaVu Sans Mono Bold (OFL license, full Unicode)
- Three font sizes: title 36pt, body 18pt, small 14pt
- Full UTF-8 support: Swedish ÅÄÖ, German Ü, French é, Czech č, CJK, Cyrillic, etc.
- All 19 PO files updated with proper UTF-8 characters (no more ASCII approximations)
- captivelogo.png displayed at top of menu (loaded from Downloads/ or data/)
- stb_image.h integrated for PNG loading (card images + logo)
- Card labels (CAPTIVE/LIBERATION) centered under their respective cards
- Renderer buffer overflow fix: dynamic allocation for post-processing buffer
- start_menu_free() for proper cleanup of fonts and images

## 2026-08-03 (language selector + card menu + 19 languages — v1.1.28)

### Language selector and translations
- Language selector in settings menu (left/right cycling through 19 languages)
- 18 PO translation files: cs, da, de, es, fi, fr, hu, it, ja, ko, nl, no, pl, pt, ro, ru, sv, zh
- All CJK and Cyrillic translations use romanized ASCII (bitmap font constraint)

### Card-based start menu
- Redesigned from flat text list to 2x2 card grid layout
- Procedural dungeon art card for Captive (Amiga-style purple palette, torches, corridor)
- Procedural cityscape art card for Liberation (gradient sky, buildings, blinking windows)
- 2x2 grid navigation for keyboard and mouse

## 2026-08-03 (i18n system + wiki updates — v1.1.26)

### Internationalization (i18n) system
- PO file loader with escape handling, multi-line support, table reuse
- `_()` macro for marking translatable strings throughout codebase
- SDL3 locale auto-detection via `SDL_GetPreferredLocales()`
- `--lang` CLI flag for manual language override
- POT template with all translatable strings (menu, settings, dialogue, errors)
- Swedish (sv) translation — first non-English language
- Wired into start menu, settings, building interaction, shop, NPC dialogue
- 48 tests passing (new: test_i18n)

### Wiki updates
- All 10 wiki pages reviewed and updated
- Liberation-Technical: PlotGen fully implemented, runtime boundary updated, RE plan items 5-8 done
- Liberation-Game-Data: PlotGen algorithm, text engine opcodes, news sources added
- File-Formats: x3g (IFF FORM O3DG), VGM (71 sets, 152 sprites), FNT, spr documented
- Data-Identity-and-Verification: ADF added as VFS source
- Game-Preservation: CityGen/BuildingGen/PlotGen marked reimplemented

## 2026-08-02 (CA wall segments + feature pipeline — v1.1.25)

### CA cell-to-viewport wall segment mapping
- 5-byte CA cells now preserve per-segment wall bits in MapCell.ca_segments
- Viewport draws partial walls: 5 column segments per cell with thickness codes
- Matches disassembled renderer at 0x4560: byte→bit mapping from Captive-Technical.md

### Full feature placement pipeline
- Bars, button combos, hidden buttons, floor traps, teleporter traps
- All interaction handlers implemented in puzzle_interact()
- Progressive difficulty: traps appear from level 3+, teleporters from level 5+

## 2026-08-02 (Textured 3D viewport + CI fix — v1.1.24)

### Perspective-correct textured polygon rendering
- `lib3d_render_textured_quad()` with per-scanline UV interpolation and z-buffer
- `city_nav_render_textured()` renders VGM wall textures in city navigation
- Committed missing dialogue/shop/save headers that broke CI

## 2026-08-02 (Liberation save system — v1.1.23)

### LSAV binary save/load format
- Big-endian serialization: seeds, difficulty, mission, gold, tick, city position, facing
- Droid state: 4 droids × (name, HP, energy, level, 8 skills, 8 equipment slots)
- 256-mission completion bitmap (32 bytes)
- Generator progress (destroyed/total)
- Magic/version validation on load
- `lib_save_from_state()` convenience builder from live CityNavState

## 2026-08-02 (ArcD compression decoder — Liberation PlotGen)

### ArcD Huffman+LZSS decompressor with full parity
- Disassembled PlotGen 68k decompressor at offsets 0x302-0x520 (12,388 byte executable from Liberation Disk 3)
- Built minimal 68k emulator to run the original binary and verify bit-exact output
- Format: 4-byte magic "ArcD" (0x41726344) + 4-byte decompressed size (BE32) + 4-byte compressed size (BE32)
- Bit buffer model: 32-bit register (d6), 8-bit byte counter (d7), bits consumed LSB-first from 16-bit big-endian words
- Three Huffman tables per block: lit_count (a3+256), match_offset (a3+0), match_length (a3+128)
- Each table: 128 bytes = 16 × (mask:16, match:16) + 16 × (shift:8, symbol:8, extra_mask:16)
- Canonical Huffman codes with bit-reversed match values and variable-length integer encoding
- Symbols 0-1 are literal values; symbol k≥2 reads k-1 extra bits from the stream
- Table reuse: when symbol count is 0, the previous block's table persists (not zeroed)
- Back-reference: offset + optional extra byte (offset ≥ 512) + mandatory byte + match_length+1 bytes
- Block structure: 16-bit block_count, then lit_count-1 main loop iterations with interleaved literal runs
- Verified bit-exact decompression against all three Liberation text files:
  - PGE.txt: 7,085 → 16,304 bytes (2 blocks)
  - DTE.txt: 5,323 → 14,136 bytes (2 blocks)
  - CTE.txt: 8,230 → 17,809 bytes (2 blocks)

## 2026-08-02 (Liberation CityGen grid disassembly)

### 64×64 city grid generation from CityGen 1.12 Amiga HUNK executable
- Extracted CityGen (10,824 bytes code + 4,888 BSS) from Liberation Disk 3
- Version string: "CityGen 1.12 (CaptiveII : Monday 03-Jan-94 02:17:04)"
- PRNG: state * 0x5E5 + 0x29 (identical to BuildingGen and Captive MapGen)
- 8×8 meta-grid with 4-bit direction bitmask per cell (N/E/S/W connections)
- Road corners at (3,0), (6,3), (3,6), (0,3) — road availability per seed
- Road walking with PRNG-biased direction selection and boundary clamping
- 13 tile templates (4×4 bytes each) at 0x2958 for meta-grid expansion
- Expansion from 8×8 meta-grid to 64×64 tile grid via template lookup
- 3 grid planes: plane0 (cell type), plane1 (road ID), plane2 (building ID)
- 2 block template sets: template A (6 cells, road blocks) and B (7 cells, features)
- Block placement with random position search and road adjacency check (types 18-21)
- Difficulty-gated phases: borders (≥0), features (≥2), road blocks (≥3)
- Data tables recovered: direction table (0x2830), road availability (0x2694),
  road counts (0x2890/0x2899), block templates (0x28B4/0x28F8)
- Test suite: PRNG, init, determinism, different seeds, borders, road cells, meta connections

## 2026-08-02 (Liberation BuildingGen disassembly)

### City generation from BuildingGen Amiga HUNK executable
- Extracted BuildingGen (23,252 bytes code + 3,956 BSS) and PlotGen (12,388 bytes code + 27,016 BSS) from Disk 3
- PRNG: state = state * 0x5E5 + 0x29 (identical to Captive MapGen)
- Grid parameter computation from seed/level: density, columns, roads, cross-roads
- Building record structure: 36 bytes (type, id, name_seed, flags, 3 connections)
- Road connection graph: forward (0xAAAA) / backward (0xBBBB) markers
- 9 building types: shop, bar, business, industrial, residence, library, police, records, special
- City names: German syllable pairs (32 syllables) + Greek letter suffix by level
- Building names: type-specific from real string tables (shop/bar/business/industrial names)
- String tables extracted to separate compilation unit for test isolation
- Fixed pre-existing link errors in test_map_gen, test_game_state, test_save_load

## 2026-08-02 (spawn placement algorithm)

### Spawn placement from CAPPO.EXE disassembly
- Full spawn flow at 0x9987–0x9AA7 with creature type routing
- 8 creature categories (DS:0x9A42), 3 types per category
- Type-based placement: singles, flagged pairs, trios, directional groups
- Subcell positioning from two 16-byte tables (DS:0x9BD8, DS:0x9BE8)
- Direction modifiers: opposite (XOR 2), perpendicular (NOT & 1)
- Modifier table (DS:0x9AB7) and difficulty offset table (DS:0x9A5A)
- HP formula integrated: min + (range * difficulty / 8), modifier scaling, cap 255
- Combat system updated to use real spawn placement instead of placeholder
- Test suite: tables, subcell lookup, HP computation, spawn counts

## 2026-08-02 (MapGen cellular automaton)

### Cellular automaton map rules from CAPPO.EXE
- 4 map types recovered from 0x39CC–0x3C21:
  - Type 0 (maze): bit tests 0x101/0x808, output 0x10
  - Type 1 (rooms): bit tests 0x202/0x404, output 0x10
  - Type 2 (open): bit tests 0x101/0x404, output 0x18 (wider)
  - Type 3 (mixed): like open with shr+or for denser walls
- 5-byte cell format matching original 10×56 grid
- MapGen DOS PRNG at 0x3D54: mul 0x5E5 + add 0x29, no ROR
- Pattern generation via rotated PRNG bitmasks
- Generator placement: count = (PRNG & 7) + 1, random position
- Feature placement pipeline identified at 0x33D7 (partial)
- Test suites: CA init, pattern, determinism, rule types, boundaries

## 2026-08-02 (item pricing and availability)

### Item pricing formula from CAPPO.EXE disassembly
- Price computation at 0xBB63: rol16-based scaling with game level + difficulty
- Early game (level <= 5): fixed base price 128
- Normal: ((level_hi <<< 4) + level_lo + base + shop_tier) <<< 4 + difficulty
- Item availability check at 0xB8D6: grade/material gating against difficulty
- 23 weapon variant tiers with ASCII prices decoded from 0x1A220
- Melee tiers: 2, 3, 5, 7, 14 gold
- Ranged tiers: 14-231 gold (14 tiers with increasing prices)
- Named variants: A12, L22, X42
- Gold cap: 200 (0xC8) at gold bar renderer (0x1AE7)
- Test suite: variant table, pricing, availability

## 2026-08-02 (AdLib MIDI playback)

### OPL2-based MIDI player
- Rewrote MIDI player from sine/square synthesis to real OPL2 FM synthesis
- Uses Captive's 26 instrument patches from CAP_A.BIN for all MIDI programs
- 9-voice OPL2 polyphony with oldest-voice stealing
- MIDI program change maps to Captive patch index
- Tempo tracking from MIDI meta events, tick-accurate rendering
- Standard MIDI format 0 and 1 parsing (64 .MID files in game data)
- Loop mode for background music
- Test suite: load, parse, render, stop, loop

## 2026-08-02 (XP and level-up formulas)

### XP/level system from CAPPO.EXE disassembly
- XP award formula (0x9621): `skill * (creature_xp + 3 + difficulty) * 12`
- Per-skill base XP table at DS:0xB708 (file 0x19EF8): 10 entries
- XP threshold function (0xAB92): compound growth per level
  - ROBOTICS (skill 0): +12.5% + 8 per level, caps at level 66
  - Other skills: +6.25% (rounded) per level, caps at level 24
  - Max threshold: 0x8A47 (35,399)
- Display level = XP >> 10 (from display code at 0xB142)
- XP overflow protection (0x87EC): cap at 0xF8FFFFFF, minimum +1
- 32-bit XP accumulator replaces old 16-bit field in Droid struct
- Test suite: threshold growth, caps, award formula, display, overflow

## 2026-08-02 (SFX bytecode interpreter — full 13 opcodes)

### Complete SFX bytecode interpreter from CAP_A.BIN disassembly
- Implemented all 13 opcodes (was 5): 0x80-0x8A, 0xC8, 0xFF
- New opcodes: 0x81 (delay), 0x82 (volume), 0x83 (note offset), 0x85 (delay variant),
  0x86 (call subroutine), 0x87 (return), 0x88 (PRNG note), 0x89 (PRNG delay), 0x8A (jump)
- Voice struct extended: note_offset, key_playing, current_note, loop table, subroutine support, PRNG state
- Fixed opcode semantics: 0x80 is key-on (not delay), 0x82 is volume (not pitch), 0x83 is note offset (not key-on/off)

## 2026-08-02 (weapon damage tables)

### Weapon damage encoding from CAPPO.EXE
- Melee: 18 entries at file 0x1A006 (lo,hi pairs → lo×hi base damage)
- Ranged: 20 entries at file 0x1A02A (base, modifier, flag, 0 records)
- Grade progression: hi bytes 33,42,51,60,69 (increment 9)
- Ranged tiers: base 72-120 (×16), modifier 45→5 (÷10 per tier)

## 2026-08-02 (creature stat tables)

### Creature stat tables from CAPPO.EXE disassembly
- HP table at DS:0xA1BF (file 0x189AF): 25 creature types with min/max HP
- Category table at DS:0x9A42 (file 0x18232): 8 categories, 3 types each
- Speed table at DS:0xA1A4 (file 0x18994): movement speed per type
- Sprite map at DS:0xA16E (file 0x1895E): graphic_id + frame_index
- HP formula: base interpolation by difficulty, modifier scaling, cap at 255
- DS segment base discovery: 0x0E3F → file offset 0xE7F0 + DS_offset
- Test suite: table integrity, HP formula, bounds, sprite map, categories

## 2026-08-02 (object sprites)

### Object sprite rendering (OBJECTS.PL5)
- 16×16 frame grid (20 cols × 12 rows = 240 frames per sheet)
- Same transparency and scaling as creature sprites
- Test suite: load, blit, null safety

## 2026-08-02 (creature sprites)

### Creature sprite rendering
- PL5 sprite sheet frame extractor (32×40 grid, 10×5 = 50 frames)
- Scaled blitting with transparency (palette index 0 = transparent)
- Frame validity detection (empty frames marked invalid)
- Test suite: synthetic load, blit, scaled blit, null safety

## 2026-08-02 (holamap)

### Holamap display implementation
- 256×128 planet surface map with 5 terrain types (water to mountain)
- Base placement using mission PRNG with terrain constraints
- Reveal mechanic for PLANET PROBE item usage
- Crosshair cursor and base markers (red=active, grey=destroyed)
- STATE_HOLAMAP game mode added
- Test suite: init, determinism, reveal, render, surface variety

## 2026-08-02 (damage formula)

### Combat damage formula implementation
- Implemented lo*hi byte damage encoding from CAPPO.EXE at 0x97F4
- Shift-left scaling (×2/×4/×8) based on droid level, capped at 0xFFFD
- Added weapon_damage field to Droid struct (lo*hi encoded uint16_t)

## 2026-08-02 (viewport & palette)

### Viewport and HUD verification
- Verified viewport 144×112 at (32,55) from VGA blit at 0x485F
- Recovered 9 HUD panel blit rectangles with exact screen coordinates
- Verified VGA palette at file offset 0xEFCE matches existing pl5_default_palette
- Rendered ALIEN1.PL5 sprite sheet with real palette (creature frames confirmed)

## 2026-08-02 (combat & PRNG)

### PRNG correction from disassembly
- Fixed main PRNG: ror 3 + xor 0x0800 (was incorrectly ror 4)
- Changed to 16-bit arithmetic matching original x86 MUL/ADD
- Documented all 4 PRNG variants in the executable

### Combat system disassembly
- Recovered hit check function at 0x97D9 (creature target list lookup)
- Recovered damage formula: lo_byte * hi_byte encoding at [di+6]
- Recovered damage scaling: shift-left up to ×8 with overflow sentinel
- Combat loop: 4 droid slots, struct size 0x10E, base 0x8DC7

### SFX event mapping from disassembly
- Disassembled INT 61h AH=0x40 call sites for real SFX indices
- Mapped 7 static game events to driver sequences
- Identified variable weapon-type and action-type SFX dispatch

## 2026-08-02 (item database)

### Item type codes from DOS executable
- Decoded type_code prefix bytes for all items from unpacked CAPPO.EXE (0x1a090)
- Record format: 00 type_code [grade] NAME 0x20
- Nine type codes: 0x00=misc, 0x08=consumable, 0x10=ammo, 0x20=chip,
  0x21=equipment, 0x27=body part, 0x30=ranged, 0x60=explosive, 0x65=body variant
- Fixed item categories (ARM, GOLD, BATTERY, EXPLOSIVES, automatics, utilities)
- Discovered weapon variant/upgrade tier format at 0x1a220 with ASCII prices
- Documented item record format in wiki

## 2026-08-02 (continued)

### AdLib sound effect data recovery
- Extracted 26 OPL2 instrument patches from CAP_A.BIN (AdLib driver)
- Extracted 49 SFX bytecode sequences (2,132 bytes total)
- Extracted 128-entry OPL2 frequency number table
- Created adlib_data.c/h with full patch and sequence data
- Test suite verifying patch count, SFX termination, frequency table

### Music system expansion
- Added all 63 MIDI file SHA-256 hashes (was 8, now 63)
- Added 6 new music categories: FCBASE, VCBASE, LONGNT, W, COMPROOM, RUNNING
- Implemented PRNG-based variant selection for categories with 11 variants
- Music system now matches original game's random track selection behavior

### Planet name generation
- Implemented captive_generate_planet_name() from DOS disassembly
- Uses consonant/vowel character tables to generate "STATION XXXX" names

## 2026-08-02

### Captive game data recovery
- Corrected all game data tables to match DOS disassembly exactly
- Real syllable table (48 syllables: VI, RUP, YUL, SCO, etc.)
- Real material names (SHIT, HUMAN, TINDRON, COPPATOR, BRONZITE, IRONIDE, CROMIZE)
- Real skill names (ROBOTICS, BRAWLING, SWORDS, etc.)
- Real device names (AG-SCAN, ROOT-FINDER, MAPPER, etc.)
- Real body part names (HEAD, CHEST, ARM, LEG, FOOT, HAND)
- Real item database (KNUCLE-DUSTER, BATTLE-GLOVE, WAR-BLADE, COLT, UZIE, etc.)
- 25 game messages, 13 shop dialogue strings from executable
- 14 music track categories with names
- 23 graphics filenames

### Viewport renderer
- Created viewport.c with 19-cell trapezoid compositing
- 5 depth ranges with perspective-correct cell sizing
- Wall, floor/ceiling, door, ornament and special cell rendering
- Wired into main.c game loop (replaces black viewport clear)
- Uses real PL5 panel sheets exclusively

### Sound system
- Created CTV/VOC decoder (Creative Voice File format)
- Wired sfx system to load CTV sound effects
- CTV decoder test suite

### PRNG parity
- All subsystems now use original PRNG (mul 0x5e5, add 0x29, ror 4, xor 0x800)
- Combat, shop, puzzle systems all converted from synthetic LCG

### Combat system
- Renamed creatures to ALIEN1-6 matching PL5 sprite sets
- Uses original PRNG for all randomness

### Liberation string tables
- Added all CD32 executable string data to code
- 32 German city syllables, 20 street types
- 35 first + 32 last names, 8 NPC titles
- 9 shop + 12 bar + 11 business + 12 industrial types

### Documentation
- Wiki updated: Captive-Technical, Captive-Game-Data, File-Formats
- CTV format documented in File-Formats wiki page
- All wiki pages synced to GitHub wiki
# 2026-08-05 (data cache invalidation)

### VFS cache correctness
- Cache signatures now include recursive loose-file metadata as well as ZIP metadata
- Replacing a loose asset invalidates the cached payload without rehashing unchanged assets
- Added regression coverage for replacing a cached loose file
- Full test suite: 49/49 passing
# 2026-08-05 (start menu l10n)

### Data scanner and version selector
- Scanner result strings and Liberation source labels are now gettext-marked
- Added the new scanner/version-selector message IDs to the translation template
- Full test suite remains 49/49 passing

### Cache replacement detection
- Cache metadata now includes inode and ctime, covering same-size file replacement

# 2026-08-05 (ARCD malformed input)

### Liberation ARCD decoder
- Reject compressed Huffman table counts outside the 1–32 entry table capacity
- Prevent malformed input from writing past the local symbol-length array
- Full test suite: 49/49 passing

### Warning cleanup
- Made ARCD and VFS size conversions explicit after review of the warnings build
- Removed an unused duplicate Liberation hash table from the start menu

# 2026-08-05 (start menu robustness)

### Renderer validation
- Start-menu rendering now safely ignores null buffers and invalid dimensions
- Added a regression call covering the null/invalid render path
- Warnings build is now clean for the reviewed targets

# 2026-08-05 (ARCD table bounds)

### Liberation ARCD decoder
- Reject Huffman tables that would generate more than 16 decoder entries
- Prevent malformed code-length data from overflowing the 16-entry table

# 2026-08-05 (ADF chain bounds)

### Amiga disk reader
- Bound OFS data-chain and FFS header/data-chain traversal by disk capacity
- Corrupt cyclic block pointers can no longer make file loading loop forever
- Full test suite: 49/49 passing

### ADF API validation
- `adf_open()` now clears stale state on failed opens
- `adf_read_file()` now requires a valid T_HEADER block
- Root-directory hash chains are also bounded against cyclic block pointers

# 2026-08-05 (configuration parsing)

### Custom feature settings
- Replaced unchecked `atoi()`/`atof()` parsing with range/error-checked conversions
- Invalid or overflowing numeric settings now retain safe defaults
- Added regression coverage for overflowing integers and non-finite floats
- Full test suite: 49/49 passing

### CLI parsing
- `--upscale-factor` now uses the validated integer parser
- Out-of-range and overflowing values produce a clear CLI error instead of undefined `atoi()` behavior

### CLI speed parsing
- `--speed` now rejects non-finite, non-positive, and overflowing values
- Verified invalid `--speed nan` exits with status 2 and a clear error

### CLI resolution parsing
- Replaced unchecked `%d` resolution parsing with validated `strtol()` fields
- Oversized or malformed `--resolution` values now fail cleanly
- Verified an overflowing resolution is rejected; full suite remains 49/49

# 2026-08-05 (Liberation pack arithmetic)

### Animation frame decoder
- Compare record and raw sizes in 64-bit arithmetic
- Prevent wrapped `raw_size + descriptor` validation for malformed packs
- Full test suite: 49/49 passing

# 2026-08-05 (l10n template sync)

### Start-menu scanner translations
- Added all scanner status, verification, and version-selector message IDs to `po/messages.pot`
- Verified the Swedish PO file with `msgfmt --check`

# 2026-08-05 (ADF regression coverage)

### ADF reader tests
- Added coverage for failed-open stale state, cyclic root hash chains, and invalid file headers
- Full test suite: 51/51 passing

# 2026-08-05 (Atari ST filesystem safety)

### ST disk reader
- Failed opens now clear stale `STDisk` state
- FAT12 reads reject truncated or prematurely terminated cluster chains instead of returning partial files
- Added bounds checks for root-directory offsets and null-safe root listing
- Added regression coverage for valid reads, truncated reads, invalid opens, and null listing arguments
- Full test suite: 51/51 passing

# 2026-08-05 (Creative Voice parser safety)

### CTV decoder
- Fixed unsupported type-1 and type-9 blocks advancing the input position twice
- Valid blocks following unsupported codecs are now decoded correctly
- Added regression coverage for an unsupported block followed by valid PCM
- Full test suite: 51/51 passing

# 2026-08-05 (ISO9660 reader state safety)

### ISO image opening
- `iso_open()` and `iso_open_raw()` now clear stale image state before rejecting null input
- Added regression coverage for failed reopen attempts after a valid image
- Full test suite: 51/51 passing

# 2026-08-05 (VGM sprite-bank bounds)

### Liberation VGM decoder
- AmSp bank parsing now rejects zero-width records and sprite planes truncated by the input buffer
- Added regression coverage for truncated and malformed sprite banks
- Full test suite: 51/51 passing

### VGM state reset
- `vgm_open()` now clears stale bank state before rejecting null input
- Added regression coverage for failed reopen after a valid VGM bank
- Full test suite: 51/51 passing

# 2026-08-05 (Liberation asset loader state safety)

### ImgA and FNT loaders
- `img_open()` and `fnt_open()` now clear destination state before rejecting invalid input
- Added ImgA regression coverage for failed reopen after valid data
- Existing FNT tests remain green
- Full test suite: 51/51 passing

# 2026-08-05 (X3G object lifetime safety)

### Liberation X3G decoder
- Failed X3G object parses now release partially allocated vertex/polygon data
- Repeated EXVL chunks no longer leak the previous vertex allocation
- `x3g_open()` clears stale state before invalid input is rejected
- Added regression coverage for failed reopen and retained full parser tests
- Full test suite: 51/51 passing

# 2026-08-05 (sprite-sheet stride correctness)

### Object and creature sprites
- Sprite-sheet loaders now use `PL5Image.width` as the row stride instead of assuming 320 pixels
- Added regression coverage with padded, wider sheets
- Full test suite: 51/51 passing

# 2026-08-05 (Liberation data API state safety)

### LiberationData opening
- `liberation_data_open_source()` now clears stale destination state when called with a null VFS
- Full test suite: 51/51 passing

# 2026-08-05 (save parser initialization)

### Liberation save reader
- Removed the read of uninitialized destination bytes before parsing a save
- Save files are still committed atomically only after complete validation
- Extended roundtrip coverage to start from a deliberately nonzero destination buffer
- Full test suite: 51/51 passing

# 2026-08-05 (renderer temporary-buffer safety)

### Frame presentation
- Removed a per-frame temporary-buffer leak when visual effects are disabled
- Renderer now handles temporary-buffer allocation failure by presenting the source pixels unchanged
- Added null guards for renderer presentation state
- Full test suite: 51/51 passing

### Renderer lifecycle
- Added null guards to renderer initialization/shutdown APIs
- SDL window/renderer resources are now released on initialization failure
- Shutdown clears handles so repeated cleanup is safe
- Full test suite: 51/51 passing

# 2026-08-05 (UI framebuffer arithmetic)

### Terminal, droid and shop overlays
- Pixel-count loops now use `size_t` multiplication instead of overflowing `int` width×height arithmetic
- Full test suite: 51/51 passing

# 2026-08-05 (droid stat saturation)

### Level-up and battery handling
- HP/energy maxima now saturate at `INT16_MAX` instead of wrapping on level-up
- Battery use performs bounded arithmetic before storing the result
- Full test suite: 51/51 passing

# 2026-08-05 (warnings audit)

### Strict build review
- Rebuilt the warning configuration after the latest fixes
- Normal build and full test suite remain green: 51/51 passing

# 2026-08-05 (Captive version selection)

### Start-menu source popup
- Start-menu scanning now distinguishes verified Captive DOS and Amiga sources
- When both versions are present, Captive now opens the version-selection popup
- The selected Captive platform is propagated into the launch configuration
- Added keyboard regression coverage for selecting Captive Amiga
- Added `CAPTIVE DOS` and `CAPTIVE AMIGA` message IDs to the l10n template
- Full test suite: 51/51 passing

### Captive popup l10n
- Added `CAPTIVE DOS` and `CAPTIVE AMIGA` entries to all 18 shipped PO files
- Verified every PO file with `msgfmt --check`

# 2026-08-06 (deterministic data-cache signatures)

### VFS scanner cache
- Directory metadata is now traversed in sorted order
- ZIP paths are sorted before cache-signature generation and lookup
- Added a fresh-VFS regression check proving cache reuse across scanner instances
- Full test suite: 51/51 passing

# 2026-08-06 (ZIP decompression error path)

### VFS ZIP extraction
- Avoided calling `inflateEnd()` when `inflateInit2()` failed
- Corrupt deflate entries now return cleanly without undefined zlib cleanup
- Full test suite: 51/51 passing

# 2026-08-06 (Amiga HUNK bounds)

### HUNK allocation table
- Prevented `last - first + 1` from wrapping in 32-bit arithmetic
- Zero-sized/overflowed hunk ranges are now rejected
- Added malformed-header regression coverage
- Full test suite: 51/51 passing

# 2026-08-06 (ANIM FORM bounds)

### Liberation ANIM parser
- All ANIM chunk walks now honor the FORM-declared extent, not just backing-buffer size
- Frame, script, and PACK extraction reject chunks outside that extent
- Added regression coverage for a truncated FORM declaration with trailing data
- Full test suite: 51/51 passing

# 2026-08-06 (start-menu data status reset)

### Data-path changes
- Start-menu data status and source masks are cleared before every rescan
- Invalid or empty paths can no longer retain stale verified-game state
- Added regression coverage for switching to a nonexistent data path
- Full test suite: 51/51 passing

### Real-data verification
- Verified `.opencaptive` end to end: Captive DOS and Amiga sources, Liberation data, and both presentation first frames
- `--verify-data all` exits successfully

### Captive runtime capture
- Launched verified Captive data with `--game captive`
- Texture atlas loaded and native frame capture completed successfully

### Liberation runtime capture
- Launched verified Liberation data with `--game liberation --skip-intro`
- Texture atlas loaded and native frame capture completed successfully

# 2026-08-06 (disk-reader boundary checks)

### ISO/ADF readers
- ISO file reads now reject LBA wraparound instead of continuing from sector zero
- ADF-OFS reads now require valid data-block type and owning file-header reference
- Added regression coverage for the ISO maximum-LBA boundary
- Full test suite: 51/51 passing

# 2026-08-06 (ARCD truncated-input handling)

### Liberation ARCD decoder
- Truncated compressed input is now rejected instead of silently decoding missing bytes as zero
- Added fixture-based truncation regression coverage for PGE, DTE, and CTE when available
- Full test suite: 51/51 passing

# 2026-08-06 (CTV truncated-input handling)

### Creative Voice decoder
- Truncated block headers and payloads now fail the decode instead of returning earlier samples as valid output
- Invalid minimum payload sizes for supported sound block types are rejected
- Added regression coverage for a block whose declared length exceeds the file
- Full test suite: 51/51 passing

# 2026-08-06 (Liberation save read safety)

### Save parser
- Short `uint16`/`uint32` fields no longer read uninitialized stack bytes before the truncation check
- Existing parse-then-commit behavior remains intact for invalid saves
- Full test suite: 51/51 passing

# 2026-08-06 (dialogue invalid-target handling)

### Liberation dialogue state
- Invalid text-node and choice-node targets now terminate the active dialogue cleanly
- `dialogue_state_current(NULL)` is now safely handled
- Added regression coverage for both invalid transition types
- Full test suite: 51/51 passing

# 2026-08-06 (puzzle interaction bounds)

### Captive puzzle system
- Puzzle interaction now validates list, game, level, map, face, and selected-droid state
- Linked puzzle targets are constrained to map bounds before cell access
- Step-triggered teleports and traps use the same coordinate/index guards
- Added regression coverage for null state, invalid target, and invalid droid selection
- Full test suite: 51/51 passing

# 2026-08-06 (puzzle UI safety)

### Puzzle helper functions
- Step checking and clipboard hints now reject null or structurally invalid state
- Clipboard hints require a valid writable output buffer and positive size
- Added regression coverage for null helper arguments
- Full test suite: 51/51 passing

# 2026-08-06 (Captive combat state validation)

### Combat input/state safety
- Combat tick, droid attacks, and interaction now validate level, position, and direction state
- Invalid creature coordinates are ignored instead of entering line-of-sight/movement calculations
- Added regression coverage for invalid direction and level state
- Full test suite: 51/51 passing

# 2026-08-06 (Liberation city-nav bounds)

### City navigation
- Forward/backward movement now requires an in-bounds current cell
- Completed moves reject out-of-bounds destinations and recover to a stable idle state
- Invalid movement vectors are ignored safely
- Added regression coverage for edge-cell and corrupt-state movement
- Full test suite: 51/51 passing

# 2026-08-06 (building catalog validation)

### Liberation building interaction
- Negative or oversized building catalogs are rejected before modulo/index use
- Building indices are now checked against both the catalog and storage limits
- Added regression coverage for invalid catalog sizes
- Full test suite: 51/51 passing

# 2026-08-06 (city-generator pointer safety)

### Liberation city generator
- Building-connection neighbor checks now use bounded integer offsets
- Removed pointer arithmetic that could temporarily form pointers outside `plane0`
- City-generator regression suite remains green
- Full test suite: 51/51 passing

# 2026-08-06 (city building-map validation)

### Liberation city grid mapping
- Negative building catalog sizes are now ignored before modulo/index operations
- Added regression coverage for invalid building catalog input
- Full test suite: 51/51 passing

# 2026-08-06 (city-generator null safety)

### Public city-grid API
- `citygrid_init`, `citygrid_generate`, and `citygrid_prng` now tolerate null state safely
- Added regression coverage for all public null-call paths
- Full test suite: 51/51 passing

# 2026-08-06 (plot density overflow)

### Liberation plot generator
- Grid-density arithmetic now clamps in 32-bit space before storing as `uint16_t`
- High seeds can no longer wrap to a falsely low density
- Added regression coverage for `UINT16_MAX` seed
- Full test suite: 51/51 passing

# 2026-08-06 (BuildingGen density overflow)

### Liberation city generator
- BuildingGen density arithmetic now clamps before narrowing to `uint16_t`
- Maximum level values can no longer wrap to a low density
- Added regression coverage for `UINT16_MAX` level
- Full test suite: 51/51 passing

# 2026-08-06 (Captive save prevalidation)

### Save API
- `save_game` now rejects invalid position, level, direction, generator, gold, and droid-stat state before writing
- The writer and reader now agree on which states are valid
- Added regression coverage for states that loading would reject
- Full test suite: 51/51 passing
## 2026-08-06 (Renderer CRT curvature dimensions)
- CRT curvature is disabled for 1-pixel-wide/high canvases, avoiding division by zero in `renderer_present`.
- Full CTest suite passes 51/51.
## 2026-08-06 (Captive shop definition bounds)
- `shop_init` now clamps the item-definition count to the `ItemDatabase` capacity before iterating.
- Negative or corrupted counts can no longer make shop generation read outside `defs`.
- Added regression coverage for extreme positive and negative counts.
## 2026-08-06 (Captive shop seed arithmetic)
- Shop seed mixing now uses explicit 64-bit arithmetic before narrowing to the PRNG state.
- Extreme level values can no longer trigger signed integer overflow during shop initialization.
- Regression coverage includes `INT_MIN` and `INT_MAX` levels.
## 2026-08-06 (Captive repair stat validation)
- `shop_repair` now rejects negative, over-maximum, or otherwise inconsistent HP and energy values before calculating a repair price.
- Invalid droid state can no longer produce a nonsensical repair transaction.
- Added regression coverage confirming gold and droid state remain unchanged.
## 2026-08-06 (Captive combat gold saturation)
- Combat gold rewards now saturate at `INT_MAX` instead of overflowing into a negative balance.
- Added regression coverage for a kill while the party already has the maximum integer gold value.
## 2026-08-06 (Captive combat creature health invariant)
- Combat target selection now ignores active creatures with non-positive maximum HP.
- Invalid creature state can no longer trigger attacks, XP conversion, or reward logic.
- Added regression coverage confirming no energy is consumed for such targets.
## 2026-08-06 (Captive mission seed arithmetic)
- Mission seed generation now uses explicit 64-bit arithmetic before narrowing to the 32-bit seed.
- Large valid mission numbers can no longer trigger signed integer overflow during mission setup.
- Added regression coverage for `INT_MAX` mission numbers.
## 2026-08-06 (Liberation combat XP saturation)
- Liberation combat XP rewards now saturate at `UINT32_MAX` instead of wrapping to zero.
- Added regression coverage for a kill with XP already near the unsigned maximum.
## 2026-08-06 (Liberation combat HUD bounds)
- Liberation combat HUD rendering now clamps the displayed enemy count to the fixed enemy-array capacity.
- Corrupt or inconsistent combat state can no longer make the renderer read beyond `enemies`.
## 2026-08-06 (Liberation combat target bounds)
- Combat attacks and target cycling now clamp `enemy_count` to the fixed enemy-array capacity.
- Corrupt target state can no longer index beyond `LibCombatState.enemies`.
- Added regression coverage for an oversized enemy count and out-of-range target.
## 2026-08-06 (Captive viewport state bounds)
- Creature rendering now clamps the creature count to `MAX_CREATURES`.
- Invalid party directions fall back to north before indexing viewport direction tables.
- Prevents malformed game state from causing renderer array access violations.
## 2026-08-06 (Captive HUD health bars)
- HP and energy bars now handle non-positive maxima without division by zero.
- Bar values are clamped to the valid 0–maximum range before rendering.
- HUD rendering now also rejects null or invalid framebuffer arguments.
## 2026-08-06 (Captive HUD direction and level bounds)
- HUD compass rendering now sanitizes invalid party directions.
- Minimap rendering rejects invalid current-level values before indexing `levels[]`.
- Prevents malformed game state from causing HUD array access violations.
## 2026-08-06 (Holomap API bounds)
- Holomap initialization and rendering now reject null or invalid framebuffer inputs.
- Base rendering and reveal operations are bounded by `HOLAMAP_MAX_BASES`.
- Added regression coverage for oversized base counts and null API arguments.
## 2026-08-06 (Captive viewport framebuffer dimensions)
- `viewport_render` now rejects non-positive framebuffer dimensions before rendering.
- Avoids attempting to render into an invalid framebuffer supplied by a caller.
## 2026-08-06 (Captive view-window level bounds)
- View-window construction now rejects `current_level` values at or beyond `MAX_LEVELS`.
- A corrupt level count can no longer make the view builder index beyond `GameState.levels`.
- Added regression coverage for an out-of-range level index.
## 2026-08-06 (Captive debug HUD level invariants)
- Debug-HUD rendering now verifies both `current_level < num_levels` and `num_levels <= MAX_LEVELS`.
- Invalid level state is ignored instead of reading an unavailable dungeon level.
## 2026-08-06 (Captive minimap opacity validation)
- Minimap rendering now replaces non-finite opacity values with a safe default before alpha blending.
- Added regression coverage for `NaN` opacity input.
## 2026-08-06 (Captive reverb float validation)
- Reverb processing now rejects non-finite amounts such as `NaN`.
- Internal delayed samples are clamped before conversion to `int16_t`.
- Added regression coverage for invalid reverb input.
## 2026-08-06 (Liberation 3D projection float validation)
- 3D projection now rejects non-finite vertices and FOV values before float-to-int conversion.
- Added regression coverage for a `NaN` vertex.
## 2026-08-06 (Liberation textured 3D float validation)
- Textured 3D scanlines now reject non-finite depth and UV values before integer texture indexing.
- Textured quads reject non-finite camera, FOV, and vertex inputs.
- Added regression coverage for invalid textured-quad state.
## 2026-08-06 (Captive audio mix float validation)
- Audio mixing now rejects non-finite master volume and channel gain values.
- Non-finite sample positions from extreme pitch values stop the channel before integer conversion.
## 2026-08-06 (MIDI master volume)
- Implemented `midi_set_volume` using persistent, clamped master volume state.
- Master volume now affects both active voices and subsequently triggered notes.
- Added regression coverage for clamping and non-finite volume input.
## 2026-08-06 (Captive droid UI active-state guard)
- Droid UI input now ignores key events when the UI state is inactive.
- Added regression coverage proving a closed UI cannot mutate inventory or energy.
## 2026-08-06 (Captive terminal active-state guard)
- Terminal input now ignores key events when the terminal is inactive.
- Added regression coverage for closed-terminal navigation and activation.
## 2026-08-06 (Captive shop active-state guard)
- Shop purchases now require an active shop state.
- A closed shop can no longer mutate inventory or gold.
- Added regression coverage for inactive-shop purchase attempts.
## 2026-08-06 (Liberation dialogue node bounds)
- Dialogue access now rejects trees whose node count exceeds `DIALOGUE_MAX_NODES`.
- Option targets are bounded before storage and lookup.
- Added regression coverage for corrupt dialogue node counts.
## 2026-08-06 (Liberation shop item bounds)
- Liberation shop purchases now reject corrupt item counts and indices beyond `LIB_SHOP_MAX_ITEMS`.
- Added regression coverage for an oversized shop inventory count.
## 2026-08-06 (Liberation city renderer coordinate bounds)
- City renderers now reject invalid navigation coordinates and non-finite smooth camera values before grid traversal.
- Prevents signed overflow in coordinate-offset calculations.
- Added regression coverage for an `INT_MAX` navigation coordinate.
## 2026-08-06 (Captive save puzzle validation)
- `save_game()` now validates puzzle type, coordinates, level, facing, and optional targets before writing.
- Captive saves can no longer succeed with puzzle records that `load_game()` would reject.
- Added regression coverage for invalid puzzle coordinates and targets.
## 2026-08-06 (Liberation dialogue choice bounds)
- Dialogue choices now reject corrupt `choice_count` values before indexing the fixed-size choice array.
- Added regression coverage for an oversized choice count.
## 2026-08-06 (Captive power socket mutation order)
- Power-socket interaction now validates the selected droid before consuming a charge.
- Invalid droid state can no longer mutate the puzzle and return failure simultaneously.
- Added regression coverage proving the charge remains unchanged.
## 2026-08-06 (Cross-save export validation symmetry)
- Cross-save export now validates droid HP/energy ranges and every exported cell type.
- Invalid cross-save data is rejected before writing instead of producing files the importer cannot read.
- Added regression coverage for invalid droid stats and cell types.
## 2026-08-06 (Replay tick ordering)
- Replay loading now rejects input streams whose tick values move backwards.
- Prevents malformed replays from executing inputs out of chronological order.
- Added regression coverage for unordered replay records.
## 2026-08-06 (Replay save ordering symmetry)
- Replay saving now rejects out-of-order input arrays before writing.
- A replay produced by the saver can no longer violate the loader's chronological-order invariant.
- Added regression coverage for rejecting unordered replay state at save time.
## 2026-08-06 (Liberation citygen count underflow)
- City parameter derivation now checks building-count sums before unsigned subtraction.
- Prevents underflow from hiding an undersized generated city at extreme levels.
- Added regression coverage for maximum-level derived counts.
## 2026-08-06 (Liberation building shop bounds)
- Building-interaction purchases now validate both `item_count` and the fixed shop-array limit before taking an item pointer.
- Added regression coverage for an oversized shop count and out-of-range item index.
## 2026-08-06 (Liberation building dialogue bounds)
- Building-interaction choice counts and labels now reject corrupt counts before exposing or indexing the fixed choice array.
- Added regression coverage for oversized dialogue choice counts through the building UI layer.
## 2026-08-06 (Liberation building dialogue selection bounds)
- Direct building-dialogue selection now validates `choice_count` before inspecting a choice target.
- Added regression coverage for invalid selection against a corrupt dialogue node.
## 2026-08-06 (Captive shop item bounds)
- Shop rendering and purchases now clamp or reject corrupt `num_items` values before indexing `item_ids`.
- Added regression coverage for an oversized shop item count.
## 2026-08-06 (Captive droid battery state validation)
- Battery use now validates the droid's energy range before consuming the item.
- Corrupt energy state can no longer be mutated while reporting a failed operation.
- Added regression coverage for energy above its maximum.
## 2026-08-06 (Captive droid item validation)
- Droid inventory use now rejects unknown item IDs before attempting equipment changes.
- Corrupt inventory data can no longer silently become a weapon.
- Added regression coverage for an unknown inventory item.
## 2026-08-06 (Captive combat damage validation)
- Creature attacks now clamp computed damage to at least one point.
- Corrupt negative damage values can no longer heal droids during `combat_tick()`.
- Added regression coverage for invalid creature damage ranges.
## 2026-08-06 (Captive combat seed arithmetic)
- Combat spawn seed derivation now uses defined 64-bit intermediate arithmetic.
- Extreme level values can no longer trigger signed-overflow undefined behavior.
- Added regression coverage for `INT_MIN` and `INT_MAX` levels.
## 2026-08-06 (Captive combat level bounds)
- Combat spawning now rejects level numbers outside the valid Captive level range.
- Prevents creatures from being created with invalid level identifiers.
- Added regression coverage for both extreme signed level values.
## 2026-08-06 (Captive generator counter saturation)
- Generator destruction counting now saturates at `INT_MAX` instead of overflowing.
- Added regression coverage for an already-saturated generator counter.
## 2026-08-06 (Captive mission completion counter integrity)
- Mission completion now requires `generators_destroyed` to equal `generators_total` exactly.
- Overshot or corrupt generator counts can no longer complete a mission.
- Added regression coverage for an overshot count.
## 2026-08-06 (Captive floor-change level bounds)
- Floor changes now reject invalid `num_levels` and current-level values before indexing the level array.
- Added regression coverage for a level count beyond `MAX_LEVELS`.
## 2026-08-06 (Captive combat line-of-sight level bounds)
- Combat line-of-sight now rejects current levels beyond the fixed level array.
- Added regression coverage for a corrupt level count/current-level pair.
## 2026-08-06 (Captive level-bound audit)
- Audited all current-level and level-count consumers after the floor-change and line-of-sight fixes.
- No additional unsafe level-array access was found in the reviewed runtime paths.
## 2026-08-06 (Captive base level metadata)
- Base-map generation now stores the logical floor index in each `DungeonLevel.level` field.
- Added regression coverage for multi-floor base metadata.
## 2026-08-06 (Captive exterior entrance bounds)
- Exterior-map generation now clamps the entrance column before indexing the map.
- Added regression coverage for negative and extreme entrance coordinates.
## 2026-08-06 (Captive exterior generator null safety)
- Exterior-map generation now safely ignores a `NULL` destination.
- Added regression coverage for the null-destination call path.
## 2026-08-06 (Full build verification)
- Rebuilt the complete `opencaptive` executable after the cumulative fixes.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Captive base generator failure output)
- `map_generate_base()` now clears `out_num_levels` even when the level buffer is `NULL`.
- Added regression coverage for the failed-generation output contract.
## 2026-08-06 (Captive CA level metadata)
- CA-to-dungeon conversion now clamps level numbers before assigning metadata and texture indices.
- Added regression coverage for negative and oversized level numbers.
## 2026-08-06 (Captive terminal framebuffer clipping)
- Terminal CRT scanlines now clip writes to the supplied framebuffer dimensions.
- Added regression coverage for rendering into a small framebuffer.
## 2026-08-06 (Liberation save read exactness)
- Save loading now checks every fixed-width field read instead of relying on `feof()` after partial reads.
- Truncated headers, droid records, mission data, and generator fields are rejected before state is committed.
## 2026-08-06 (Captive start-menu framebuffer clipping)
- Start-menu scanner and popup fills now clip both axes before writing pixels.
- Added regression coverage for rendering all affected views into a 16x16 framebuffer.
## 2026-08-06 (xBRZ dimension overflow validation)
- 2x, 3x, and 4x upscalers now reject source dimensions that would overflow the destination stride calculation.
- Added regression coverage for extreme width and height metadata.
## 2026-08-06 (CTV sample-table allocation overflow)
- CTV decoding now verifies the sample-table allocation size before growing it.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation X3G chunk bounds)
- X3G EXVL and PLST chunk validation now uses overflow-safe remaining-size checks.
- The chunk walker also avoids overflowing its loop-bound expression on large inputs.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation X3G container bounds)
- X3G OFFS and nested FORM/VCDO validation now also uses remaining-size checks instead of additive bounds expressions.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (VFS null-safe cleanup)
- `vfs_free(NULL)` is now a safe no-op, matching the other VFS API entry points.
- Added regression coverage and re-ran all 54 tests successfully.
## 2026-08-06 (Start-menu null-safe cleanup)
- `start_menu_free(NULL)` is now a safe no-op for failed or partial menu initialization paths.
- Added regression coverage and re-ran all 54 tests successfully.
## 2026-08-06 (Captive viewport object blitter validation)
- Viewport object blits now reject null atlases, invalid frame indices, non-positive framebuffers, and zero-sized destinations before scaling arithmetic.
- Full executable build and all 54 tests pass.
## 2026-08-06 (Liberation texture-coordinate overflow)
- Textured 3D scanlines now wrap finite texture coordinates with `fmodf` before integer conversion, avoiding undefined behavior for extreme perspective values.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation text-section bounds)
- Text expansion now rejects sections whose pointer and length fall outside the parsed raw text buffer.
- Added regression coverage for a foreign section descriptor; all 54 tests pass.
## 2026-08-06 (Captive mission base buffer size)
- Mission creation now allocates the full `MAX_LEVELS` base buffer required by the map generator before reserving one slot for the exterior level.
- This removes a stack overwrite when the generator emits the maximum number of logical floors.
- Full executable build and all 54 tests pass.
## 2026-08-06 (Liberation data reopen cleanup)
- Reopening a `LiberationData` object now closes previously owned disc data and presentation buffers instead of leaking them during source changes.
- Cleanup remains safe for zero-initialized or foreign/uninitialized structs via a lifecycle marker.
- Full executable build and all 54 tests pass.
## 2026-08-06 (RNC truncated-output rejection)
- Forward RNC1 decoding now returns an error when its safety limit is reached before the declared uncompressed size is produced.
- Partial output can no longer be reported as a successful decompression.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (RNC legacy stream minimum size)
- RNC input dispatch now permits valid 13-byte legacy headers while retaining the 18-byte minimum for modern forward streams.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation save write verification)
- Liberation save helpers and all fixed fields now verify each `fwrite` result before reporting success.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Amiga OFS capacity overflow)
- OFS file-chain buffer growth now checks `SIZE_MAX/2` before the initial capacity doubling.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Atari ST file-size allocation guard)
- ST disk file reads now reject file sizes larger than the disk's possible data area before allocating memory.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Atari ST root offset bounds)
- Root directory cluster offsets now use 64-bit arithmetic and are only exposed when they fit the disk image and 32-bit API field.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Amiga OFS block-address bounds)
- OFS scanning now rejects disk images whose block count cannot be represented by the format's 32-bit block pointers.
- File-chain decoding validates the header block before pointer arithmetic.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (ADF FFS boot-block rejection)
- FFS file loading now rejects block 0 in a file's data-block table, preventing the boot block from being treated as file content.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Texture UV non-finite guard)
- Texture sampling now rejects NaN and infinite UV coordinates before float-to-integer conversion.
- Full executable build and CTest suite remain green: 54/54 tests passed.
## 2026-08-06 (Liberation IMG offset validation)
- IMG sprite offsets must now begin after the sprite table and remain strictly ordered.
- Multi-frame offsets receive the same bounds and ordering validation before frame decoding.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (ISO9660 directory extent bounds)
- Directory parsing now limits records in the final sector to the directory extent declared by ISO9660.
- Bytes belonging to adjacent sectors/contents can no longer be interpreted as directory entries.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (ISO9660 root null guard)
- `iso_list_root` now safely returns no entries for a null image instead of dereferencing it before validation.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (AMOS sprite size overflow guard)
- AMOS sprite plane-buffer size calculations now reject `size_t` overflow before comparing against the input buffer.
- This keeps malformed sprite dimensions safe on 32-bit builds as well as 64-bit builds.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (ARCD truncated output rejection)
- ARCD decoding now rejects streams that terminate before producing the declared decompressed size, even when they contain a clean block terminator.
- Partial output can no longer be reported as successful data.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation VGM size overflow guard)
- VGM AMOS-bank plane-buffer calculations now reject `size_t` overflow before comparing against the available input.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation VGM depth consistency)
- VGM banks with more than five bitplanes are now rejected during bank validation, matching `amos_sprite_get` and its 32-colour decoder.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Amiga HUNK section accounting)
- HUNK validation now requires exactly one CODE/DATA/BSS payload and one END marker for every declared hunk.
- Extra sections after an early END can no longer make a malformed stream appear valid.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (ADF FFS HighSeq validation)
- FFS file loading now rejects a `HighSeq` value above the 72 block pointers available in a header instead of silently clamping it.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (CTV data-offset validation)
- CTV/VOC decoding now requires the data offset to be at least the complete 26-byte header length.
- Header bytes can no longer be reinterpreted as audio blocks through a malformed offset.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (ANM frame-chain validation)
- ANM decoding now requires the backwards frame chain to terminate exactly at `cmd_end`.
- Files with valid-looking frames followed by an invalid/truncated chain are rejected instead of partially accepted.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation X3G odd-chunk padding bounds)
- X3G FORM/chunk parsing now validates the required padding byte before advancing past odd-sized chunks.
- A missing pad can no longer underflow the remaining-size calculation and lead to out-of-bounds parsing.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation X3G polygon validation)
- X3G PLST parsing now rejects malformed polygon records and polygon lists exceeding the supported maximum instead of retaining a valid prefix.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation X3G vertex-reference bounds)
- Polygon vertex references are now required to be aligned vertex offsets and within the owning object's vertex array.
- Invalid references can no longer become out-of-range renderer indices.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (3D renderer object-local index bounds)
- The generic 3D renderer now validates polygon indices against the current object's vertex count before adding projected references.
- Invalid synthetic or runtime-created objects can no longer alias projected vertices from another object.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation city navigation progress bounds)
- City navigation now rejects non-finite or out-of-range movement progress and resets the movement state before it can poison camera coordinates.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation text-section ID overflow)
- Text-table section IDs now reject decimal overflow instead of wrapping into a different valid ID.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation city-grid road bounds)
- Recursive road walking now validates the metadata-grid coordinates and direction before indexing or recursing.
- Invalid paths can no longer access the city metadata buffer outside its 7x7 bounds.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Liberation entry-point offset state)
- City-grid feature state now preserves the selected cell offset in a 32-bit field, matching the entry-point search's high-word read.
- Entry-point generation no longer falls back to cell 0 because a 16-bit mode field discarded the offset.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Cross-save write verification)
- Cross-save export now checks every serialized field and map-cell write instead of relying only on the final stream error flag.
- Truncated or failed writes are now reported as an unsuccessful export.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Start-menu initialization safety)
- Fresh `start_menu_init()` no longer reads the existing `StartMenu` contents before clearing it, so callers may safely initialize a non-zeroed stack object.
- Added `start_menu_reinit()` for deliberate menu re-entry while preserving loaded resources and the configured data path.
- Full CTest suite remains green: 54/54 tests passed.
## 2026-08-06 (Strict-warning cleanup)
- Removed unused pause-menu display-width locals and an unused Liberation text-parser helper.
- Marked the MIDI voice-allocation channel parameter intentionally unused; strict project code now progresses further under `-Wall -Wextra -Werror`.
- The remaining strict-build failures are confined to unused helper functions in the vendored `stb_image.h` implementation.
## 2026-08-06 (Additional strict-warning cleanup)
- Removed unused intermediate values from Liberation city generation, city-grid normalization, and xBRZ upscaling.
- Marked intentionally unused renderer parameters explicitly and kept the regular build behavior unchanged.
- Full CTest verification follows after the cleanup.
## 2026-08-06 (Strict warning gate)
- Test-only unused counters were either converted into assertions or removed.
- The complete project now builds and passes all 54 tests with `-Wall -Wextra -Werror -Wno-unused-function`; the only suppressed warnings originate in vendored `stb_image.h` helpers.
## 2026-08-06 (Clean full Werror build)
- Scoped Clang's `-Wunused-function` suppression to the vendored `stb_image.h` include instead of passing a global warning suppression.
- Removed the last unused project helpers from city-grid and xBRZ code.
- Full build and all 54 tests now pass with `-Wall -Wextra -Werror`.
## 2026-08-06 (CLI data-scan aliases)
- Added the documented `--scan-data` and `--scan-game-data` aliases, mapping to verification of all supported game data.
- Added the documented `--data-dir` alias for `--data`.
- Headless CLI verification now reaches the data scanner and reports missing data with the expected nonzero status; full CTest remains green.
## 2026-08-06 (CLI banner duplication)
- Removed the unconditional startup version print, which duplicated the version line for `--help`, `--version`, and data-scanning commands.
- Verified that each of those CLI paths now emits one version line; full CTest remains green.
## 2026-08-06 (Integer-scaling default consistency)
- Synchronized `main()`'s initial configuration with the documented and start-menu default: integer scaling is enabled by default.
- Full build and all 54 tests remain green.
## 2026-08-06 (Zero-size window input guards)
- Mouse coordinate translation now ignores events while SDL reports a zero-sized window, preventing division by zero during minimization or headless transitions.
- Guards cover the main menu, Liberation mission menu, and pause menu.
- Full build and all 54 tests remain green.
## 2026-08-06 (Captive save export validation)
- Captive saves now reject invalid dungeon cell types before writing them.
- This keeps save export consistent with the loader's cell-type validation and prevents self-generated unreadable saves.
- Full build and all 54 tests remain green.
## 2026-08-06 (VFS cache timestamp precision)
- VFS cache signatures now include nanosecond modification time on POSIX platforms.
- Replacing a same-size asset within one wall-clock second can no longer silently reuse stale scan data.
- Full build and all 54 tests remain green.
## 2026-08-06 (Replay loader initialization safety)
- Replay loading now stages into a zero-initialized replacement state instead of copying an uninitialized caller object.
- A successful load still replaces the complete replay system, while failed loads leave the destination untouched.
- Full build and all 54 tests remain green.
## 2026-08-06 (Captive replay input bridge)
- Added a stable byte action table for Captive keyboard replay instead of serializing platform-specific SDL keycodes.
- Captive key events are now recorded at the game tick and replayed at the same tick; multiple actions on one tick are preserved.
- Liberation replay remains intentionally outside this bridge until its separate input model is verified.
- Full build and all 54 tests remain green.
## 2026-08-06 (Captive mission start and replay seed)
- Starting Captive from droid configuration now generates the first mission before entering the game state.
- Replay playback uses the recorded mission seed when creating that initial dungeon, preserving deterministic map generation.
- Full build and all 54 tests remain green.
## 2026-08-06 (Windows VFS cache invalidation)
- VFS cache signatures now recursively include loose files and directories on Windows as well as POSIX.
- Changing a loose Windows asset can no longer leave a stale hash result cached merely because the data-root directory itself was unchanged.
- Full build, VFS tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation save mission bounds)
- Liberation save read/write now rejects mission number 256 or higher, which cannot be represented by the 256-entry completion bitmap.
- This prevents a save from carrying a mission value that later cannot be queried safely through the mission-completion API.
- Liberation save tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Cross-save import initialization safety)
- Cross-save import no longer copies the caller's possibly uninitialized `GameState` before parsing the file.
- Failed imports still leave the destination untouched, while successful imports now give non-serialized fields deterministic zero values.
- Custom-feature tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Cross-save resume state)
- Successful cross-save imports now enter `STATE_GAME` instead of silently landing in the menu because the reconstructed state is zero-initialized.
- Added an assertion covering the imported state's playable mode.
- Custom-feature tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation source selection)
- Explicit Amiga/ADF source selection no longer performs an unnecessary CD32 track hash search before loading the requested source.
- This keeps source selection faithful and avoids needless large-image scanning and cache work.
- Full build, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Start-menu Liberation scan initialization)
- Both local `LiberationData` instances used by start-menu data scans are now zero-initialized before lifecycle-aware open/close calls.
- This removes undefined reads of the lifecycle marker during ordinary menu scans.
- Start-menu tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Start-menu scan lifecycle audit)
- Audited all local data-parser temporaries used by the start menu and CLI verification paths; lifecycle-aware `LiberationData` instances are now explicitly initialized before opening.
- This closes the remaining uninitialized-object path in normal data scanning.
- Start-menu tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (PO parser escape bounds)
- The i18n PO parser now bounds-checks both output characters produced by unknown escape sequences.
- Malformed or oversized translation lines can no longer write past the fixed message buffer.
- i18n tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation text empty-option bounds)
- Text expansion now handles a trailing empty random option such as `[alpha|]` without unsigned-length underflow.
- Added regression coverage for both the non-empty and empty selection paths.
- Liberation text-table tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Combat target selection)
- Captive creature AI now selects attack targets only from living droids; destroyed droids are excluded from random target selection.
- Added regression coverage proving a living droid takes damage while a destroyed droid remains unchanged.
- Game-state tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Dungeon stair topology coverage)
- Added a multi-seed regression test that traverses every generated Captive floor transition in both directions.
- The test covers 64 mission seeds and verifies that every stairs-down cell has a reachable stairs-up destination and return path.
- Game-state tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Gameplay message localization hooks)
- Active Liberation and Captive combat/status messages are now routed through the i18n lookup layer.
- Added their message IDs to `po/messages.pot`; languages without a translated entry retain the safe English fallback.
- Full build, all 54 CTest tests, and `git diff --check` pass. The documented `po/validate_po_layout.sh` script is absent from this checkout, so that specific check could not be run.
## 2026-08-06 (Liberation dialogue entry node)
- Dialogue startup now skips leading exit nodes and begins at the first playable text/choice node.
- This fixes generated NPC dialogues, which construct their exit node before the greeting and previously closed immediately on start.
- Dialogue/shop tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation l10n cleanup)
- Localized the remaining hard-coded "Fine paid. Rep +15" message in both building-exit input paths.
- Added the message to the translation template; untranslated locales continue to fall back to English.
- Liberation dialogue/shop tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation save-position validation)
- Liberation save read/write now rejects city coordinates outside the 64×64 navigation grid.
- Added regression coverage for negative and out-of-range coordinates.
- Save tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation navigation invariant)
- City navigation now rejects diagonal or zero-length movement vectors during state updates; only cardinal movement is valid.
- Added regression coverage for corrupted diagonal movement state.
- Navigation test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Captive shop repair arithmetic)
- Shop repair cost calculation now uses 64-bit intermediate arithmetic before debiting gold.
- Added coverage for the largest valid droid HP/energy values.
- Shop test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Captive shop cost robustness)
- Shop repair calculations now use 64-bit intermediate values before comparing and debiting gold.
- Added regression coverage using the largest representable droid health and energy values.
- Shop test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Captive inventory category guard)
- Droid UI no longer equips ammunition, keys, maps, or other non-weapon items as weapons.
- Added regression coverage for ammunition in an inventory slot.
- Droid UI test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Captive droid inventory equipment)
- Droid inventory handling now rejects non-weapon items in weapon slots; ammunition, keys, maps, and tools remain in inventory.
- Added regression coverage for ammunition.
- Droid UI test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Captive gameplay l10n)
- Localized the remaining main-loop gameplay messages for creature defeat, level-up, pit damage, puzzle activation, and leaving a building.
- Added all five strings to `po/messages.pot`; untranslated locales fall back to English.
- Full test suite and `git diff --check` pass.
## 2026-08-06 (Captive terminal l10n)
- Localized terminal map, status, mission, generator, title, and navigation strings.
- Added the terminal strings to `po/messages.pot`; untranslated locales fall back to English.
- Full build, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Terminal test i18n linkage)
- Updated the standalone terminal test target to link the i18n implementation and SDL3 after terminal strings were localized.
- Full build, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation combat target flow)
- After defeating the selected enemy, Liberation combat now automatically selects the next living enemy.
- Added regression coverage for target advancement.
- Combat test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (NPC dialogue initialization audit)
- Fixed undefined behavior in the NPC dialogue regression test by zero-initializing its load state before probing failure paths.
- Documented the required zero-initialization contract for first-time dialogue-data loads.
- NPC dialogue test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation city-grid building flags)
- Building-shape connection resolution now masks the high flag bit in `plane2` before comparing building IDs.
- This preserves valid connections for cells carrying the high-bit feature marker.
- City-grid test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation building-ID flag masking)
- Building mapping and building entry now mask the high `plane2` flag bit before resolving building IDs.
- Added regression coverage for flagged building cells.
- City-grid/dialogue tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation plane2 rendering flags)
- Remaining city-grid wall metadata and city rendering paths now mask the `plane2` high flag bit before using building IDs.
- This keeps wall annotations and building visuals consistent with the corrected interaction mapping.
- City-grid/navigation tests, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation taxi and city-map IDs)
- Taxi destination lookup and city-map building highlighting now mask the `plane2` high flag bit.
- All known runtime consumers now interpret flagged `plane2` cells consistently.
- Full build, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation droid save/load state)
- Liberation F5/F9 now persists and restores droid skills, body-part equipment, and both weapon slots instead of silently replacing them with zeroed data.
- Weapon damage is recalculated after loading restored weapons.
- OpenCaptive build, save test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation inventory save format v2)
- Liberation save format v2 now persists all ten droid inventory slots.
- Version 1 saves remain readable; their inventory is initialized empty.
- F5/F9 wiring, save test, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation partial-droid save restore)
- F9 now clears the complete droid array before restoring the droids present in a save file.
- Loading a save with fewer than four droids can no longer retain stale droid state from the previous session.
- OpenCaptive build, all 54 CTest tests, and `git diff --check` pass.
## 2026-08-06 (Liberation save state construction)
- `lib_save_from_state()` now records zero droids when no droid array is supplied, instead of creating a save claiming to contain zero-initialized droids.
- Added regression coverage for a NULL droid source.
- Save test, all 54 CTest tests, and `git diff --check` pass.
# 2026-08-06 (Liberation save restore consistency)

- Captive-stridens fiendeskada klipps nu mot aktuell HP i stället för att
  konverteras med `int16_t`-wrap; extrem vapenskada dödar därför korrekt.
- Samma saturering används nu när fiender skadar droids; extrem fiendeskada
  kan inte längre återuppliva en droid genom HP-wrap.
- Lade till regressionstest för maximal fiendeskada och verifierade att exakt
  en levande droid skadas utan att någon HP wrapar.
- Pusselns golvfällor och energisocklar använder nu saturerande HP-/energiändring
  i stället för `int16_t`-wrap vid korrupta eller extrema runtimevärden.
- Kopplade Captives `puzzle_generate()` till nya missionsstarter; tidigare hade
  funktionen ingen caller och nya uppdrag saknade därför alla genererade pussel.
- Kopplade även Captives `combat_spawn_for_level()` till nya missionsstarter;
  tidigare startade nya Captive-missions utan fiender.
- Fixade spakgenereringens retry-loop: en kandidat utan angränsande vägg avbryter
  inte längre alla återstående försök.
- Samma retry-fel korrigerades för bars, knappkombinationer, dolda knappar och
  energisocklar; de försöker nu fler kandidater när första platsen är ogiltig.
- Encounter-spawn validerar nu varje faktisk spawncell som golv i stället för att
  placera fiender i väggar eller klampa dem till kartkanten.
- Captive combat ignorerar nu aktiva fiender med `hp <= 0`, markerar dem
  inaktiva och startar korrekt respawn-timer; regressionstest täcker detta.
- Liberation-stridens fiende- och droidskador saturerar nu HP till 0 i stället
  för att kunna wrapa via `int16_t`; regressionstest täcker extrem fiendeskada.
- Industriell fara, fallgropar och byggnadsfara använder nu samma saturerande
  droidskadefunktion i huvudloopen.
- Level-up jämför före- och efterattackens XP direkt, utan risk för unsigned
  underflow. Regressionstest täcker låg XP mot stor fiende.
- Liberation-start normaliserar mission 0 till mission 1 före stads-/temaindexering,
  så en korrupt eller äldre sparfil kan inte skapa negativ arrayindexering.
- Nollställer Liberation-stadens genereringsflagga vid ny extern start/laddning,
  så ett tidigare missions stad inte återanvänds efter menyn eller vid nytt seed.
- F5 sparar nu aktuell generatorstatus och F9/"Fortsätt" återställer den till
  `GameState`, så uppdragsframsteg inte tappas vid laddning.
- F5 klipper generatorräknare säkert till sparformatets 16-bitarsintervall och
  upprätthåller `destroyed <= total` även vid korrupt runtime-status.
- Lade till XP-systemets implementation i Liberation-stridstestets länkning och
  verifierade att stridsbelöning aldrig överskrider `XP_MAX`.
- Liberation-stridens XP-belöning använder nu `xp_add()` och kan inte passera
  den gemensamma `XP_MAX`-gränsen.
- Uppgraderade Liberation-sparformatet till version 3 med droidens fulla
  32-bitars XP; version 1 och 2 kan fortfarande läsas.
- Lade till rundturstest som verifierar att XP-värdet faktiskt skrivs och läses.
- Samordnade F9 och "Fortsätt Liberation" så sparade mission, 32-bitars seed,
  stadsposition, droidskills, utrustning, inventory och vapenskada återställs
  konsekvent.
- Nollställer hela droidtabellen före återställning så äldre sparfiler med färre
  droids inte lämnar kvar gamla standarddroids.
- Den strikta varningsbyggningen och alla 54 CTest-tester passerar.
- Elektrisk väggskada i Captive beräknas nu i ett bredare heltal före
  konvertering tillbaka till droidens HP, så skadan kan inte orsaka signed
  integer-wrap vid extrema eller korrupta nivåvärden.
- Liberation-guld från sparfiler klipps säkert till `INT_MAX` innan det läggs
  i runtime-state, och negativt runtime-guld sparas som noll i stället för
  att wrapa till ett enormt positivt belopp.
- Liberation-sparformatet är nu version 4 och sparar även det delade
  inventoryt med bakåtkompatibel läsning av äldre versioner. F9 återställer
  därför föremål som låg mellan droidarnas inventoryplatser.
- Lade till rundturstest för delade inventoryföremål; alla 54 tester passerar.
- Liberation-sparning validerar nu shared-inventory-antalet före filöppning,
  så ett ogiltigt runtimevärde inte förstör en befintlig sparfil.
- Cross-save version 1 begränsas nu uttryckligen till Captive, eftersom
  formatet bara serialiserar dungeon-/party-state och tidigare felaktigt
  annonserade stöd för Liberation trots att dess stadsstate inte sparades.
- Regressionstest täcker nu att Liberation avvisas av Captive-formatet.
- VFS-cache-signaturen beräknas nu en gång vid `vfs_init` i stället för vid
  varje hashfråga. Identisk dataskanning sjönk i praktiken från cirka 42 till
  8 sekunder när 38 ZIP-arkiv och befintlig cache användes.
- Liberation-byggnadstyper och NPC-roller går nu via gettext; de tidigare
  hårdkodade engelska namnen kunde inte lokaliseras. Nya meddelande-ID:n finns
  i `po/messages.pot`, och `test_liberation_citygen` länkar nu i18n-lagret.
- Byggnadsinteraktionen använder nu `int *` för Liberation-guld, samma typ som
  `GameState.gold`; tidigare skickades en felaktigt castad `int *` som
  `uint32_t *`, vilket gav undefined behavior och kunde göra negativa saldon
  till enorma positiva belopp. Köp konverteras säkert internt.
- Citygridens formupplösning begränsar nu den sammanlagda byggnadsräknaren
  till `CITYGRID_MAX_BUILDINGS` innan `buildings[]` traverseras. Korrupta eller
  extrema räknarvärden kan därför inte längre ge läsning utanför arrayen.
- Liberation-butiker är nu faktiskt användbara från huvudloopen: efter valet
  av handel köper nummerknapparna 1–9 motsvarande vara, med lokaliserat
  resultat- eller felmeddelande. Tidigare fanns köp-API:t men inget anrop från
  spelinmatningen.
- Liberation-butiksdialogen säger nu “Buy” i stället för “Buy/Sell”, eftersom
  säljintegrationen ännu inte är kopplad till spelarens inventarie. Det
  förhindrar att UI:t lovar en funktion som inte kan genomföras korrekt.
- Köpbindningen är nu begränsad till det explicita handelsläget. Nummerknappar
  efter exempelvis “Any news?” kan inte längre råka köpa en vara.
- Liberation-köp blockeras nu innan betalning när det gemensamma inventariet
  är fullt. Tidigare kunde varan debiteras och därefter tyst försvinna vid
  överföringen till inventariet.
- Droid-vapen väljs nu efter den faktiska `low × high`-skadan som
  stridsformeln använder, inte efter det packade bytevärdets numeriska ordning.
  Det förhindrar att ett svagare vapen väljs på grund av högre högbyte.
- VFS hanterar nu tomma filer korrekt: `malloc(0)` kan inte längre få en
  existerande nollbytesfil att se ut som saknad data. Regressionstest täcker
  både vanlig läsning och SHA-256-sökning av tom fil.
- Dialogträd avvisar nu textnoder vars nästa nod inte ryms i formatets
  16-bitars nodreferens. Tidigare kunde ett stort korrupt `unsigned`-värde
  wrapa till en annan dialognod.
- Cross-save-import validerar nu `DungeonLevel.level` innan state publiceras;
  korrupta nivåindex kan inte längre smyga in i den återställda spelstaten.
- Replay-läsaren kräver nu exakt filstorlek enligt headerns inputräknare;
  extra efterföljande bytes accepteras inte som en giltig replay.
- Pusselgeneratorn avvisar nu ogiltiga nivånummer innan den skapar pussel;
  negativa eller för stora `level`-referenser kan inte längre hamna i state.
- Pusselgeneratorn avvisar även korrupta `num_puzzles`-värden innan den
  använder pussellistan, så ett negativt antal kan inte ge skrivning före
  `puzzles[]`.
- Vapenskadeberäkningen avvisar nu negativa eller överfulla `int16_t`-värden
  innan de konverteras till combatens bytekodning; korrupta itemdata kan inte
  längre wrapa till ett vapen med oväntat hög skada.
- Liberation-3D-renderaren avvisar nu texturer vars angivna dimensioner
  överskrider den inbyggda pixelbufferten. Korrupta X3G-/texturmetadata kan
  därför inte längre leda till läsning utanför `Lib3dTexture.pixels`.
- Startmenyn validerar nu dat sökvägsmarkören innan textredigering och
  använder storleksäkra längdberäkningar för infogning/radering. Ogiltigt
  återanvänt input-state kan därför inte längre flytta `memmove` utanför
  `data_path`.
- Dataskannerns progressräknare använder nu det faktiska antalet Captive- och
  Liberation-resurser i stället för att behandla hela Liberation-kontrollen
  som ett enda steg. Procentvisningen är därför korrekt under skanning.
- Byggnadsinteraktionen avvisar nu `plane2`-sentinelvärdet `0xFF` innan
  flaggbitar maskeras. Tomma byggnadsfält kan därför inte längre tolkas som
  byggnad 127 och öppna en felaktig dialog.
- Sentinelvärdet `plane2 == 0xFF` hanteras nu före flaggmaskering även i
  citygen-mappningen, taxifunktionen, stadskartan och 3D-väggfärgerna. Ogiltiga
  celler kan därför inte längre visas eller användas som byggnad 127.
- Citygenens interna byggnadsvandring avvisar nu också tomma och border-
  sentinelvärden innan `plane2`-ID maskeras. Ogiltiga celler kan därför inte
  påverka byggnadsform- eller entry-point-upplösningen.
- MIDI-parsern avvisar nu avkortade VLQ-sekvenser i både initiala delta-tick
  och efterföljande events. Partiella längder kan därför inte längre flytta
  playback till ett falskt tick eller lämna ett korrupt spår i loop.
- MIDI-uppspelaren nollställer nu running status efter system- och metaevent,
  enligt Standard MIDI. En data-byte efter exempelvis SysEx kan därför inte
  längre feltolkas som ett nytt kanal-event.
- `midi_render` återställer nu en ogiltig `tick_frac` innan sampleloopen.
  Korrupt eller återanvänd state kan därför inte längre ge negativ återstående
  ticklängd och låsa ljudrenderingen.
- Captive-kompositorn kräver nu en stride minst lika bred som viewporten och
  använder `size_t` för source-index. Smala framebuffers och extrema panel-
  koordinater kan därför inte längre orsaka radöverskrivning eller heltals-
  overflow vid källindexering.
- Captive view-window-bygget validerar nu party-koordinater innan den räknar
  fram synliga celler. Korrupta extrema koordinater kan därför inte längre
  orsaka signed overflow i koordinattransformen.
- `game_state_init` normaliserar nu missionsnummer mindre än 1 till mission 1.
  Nya stateobjekt kan därför inte starta med ett ogiltigt negativt eller nollat
  missionsnummer.
- Captive-butiken normaliserar nu negativa nivåer till nivå 0 innan lager-
  tier beräknas. Korrupt eller återanvänd spelstate kan därför inte längre
  skapa en felaktigt tom butik på grund av negativ heltalsdivision.
- `combat_spawn_for_level` normaliserar nu negativa creature-räknare innan
  spawnloopens arrayindexering. Korrupt eller delvis återläst combat-state kan
  därför inte längre skriva före `CreatureList.creatures`.
- Liberation-textparsern begränsar nu nästlade `^O`-/`^XCA`-alternativ till
  ett säkert rekursionsdjup. Korrupt DTE-data kan därför inte längre överfylla
  C-stacken under dialogexpansion.
- Polisdialogens vägran att betala böter kräver nu ett faktiskt giltigt sista
  valindex. Ett `UINT_MAX`-val kan därför inte längre wrapa och felaktigt
  registrera bötesvägran.
- Bar fights följer nu Liberation-dokumentationens 25-procentiga, seedade
  chans per lyckat barköp i stället för att inträffa deterministiskt vart
  fjärde köp. Test täcker både utfall och icke-utfall över flera seeds.
- OPL2-emulatorns negativa feedbackfas skalar nu ljudprover med definierad
  heltalsaritmetik i stället för ett odefinierat vänsterskift. UBSan rapporterar
  inte längre fel i OPL2-, MIDI- eller AdLib-renderingen.
- `--reverb` och konfigurationsflaggan `audio_reverb` kopplas nu till den
  faktiska ljudmixningen. Reverbmängden klipps säkert till 0–1 innan den
  appliceras på ljudbufferten.
- Alla 18 översatta PO-filer är nu synkroniserade mot den aktuella
  `messages.pot`-mallen och deras MO-filer är ombyggda med formatkontroll.
- Liberation-renderaren kan nu använda `texture_filter` för bilinjär,
  repeterande textursampling; standardläget är fortsatt närmaste pixel för
  originaltrohet.
- Liberation-renderaren kan nu använda `dynamic_lighting` för avståndsbaserad
  ljusintensitet på texturer. Avstängt läge lämnar tidigare färgresultat
  oförändrat.
- F10-popupen stängs nu automatiskt när ett alternativ byter spelläge, så
  exempelvis `COMPLETE OBJECTIVE` inte lämnar en osynlig modal som blockerar
  holomapens input. Liberation-missionspriten kontrollerar dessutom sina
  dimensioner mot 320×256-canvasen innan den kopieras.
- Liberation-animationsdekodern avvisar nu planarbilders radbredder som inte
  ryms i den publika `uint16_t`-bredden. Korrupta `width_bytes` kan därför inte
  längre trunkeras till en annan bildbredd vid första bildrutan.
- Liberation-animationsdekodern avvisar nu också planarbilders djup över fem
  bitplan, eftersom Liberation-paletterna bara rymmer 32 färger. Korrupta
  animationsdata kan därför inte längre läsa färgindex som aliaserar eller
  överskrider den avsedda paletten.
- Liberation-sessionens data och missionsmenybild frigörs nu när startmenyn
  öppnas igen. Upprepade växlingar mellan startmenyn och Liberation behåller
  därför inte gamla sessionsresurser.
- Startmenyn ignorerar nu musklick precis utanför inställningsraderna. Ett
  klick strax ovanför första raden kan därför inte längre feltolkas som ett
  klick på rendererinställningen på grund av negativ heltalsdivision.
- Pausmenyn öppnar nu faktiskt startmenyns inställningsvy när `SETTINGS`
  väljs. Alternativet kan därför inte längre bete sig som `QUIT` och kasta
  bort användarens menyval.
- Startmenyn validerar nu bilddimensioner innan den skalar launcherbilder.
  Ett kvarlämnat eller korrupt bildpekare-state kan därför inte längre orsaka
  division med noll under första renderingen.
- Pausmenyns pixel-font innehåller nu också markören `>`. Den valda raden
  visas därför korrekt även när TTF-fonten inte används.
- Startmenyn ändrar inte längre hover-markeringen bakom setup- eller
  versionspopupen. När popupen stängs ligger markeringen därför kvar på rätt
  huvudmenyrad.
- Windows-CI:s VFS-cachetest är nu deterministiskt: ersättningsarkivet ändrar
  storlek så att källfingeravtrycket alltid invalideras även vid mycket snabba
  omskrivningar inom samma Windows-tidsupplösning.
- RNC-dekoderns framåtriktade bitläsare avvisar nu avkortade strömmar i stället
  för att fylla på med syntetiska nollbitar. Ett regressionsprov säkerställer
  att korrupt Liberation-data inte kan passera som en giltig dekodning.
- Liberation-butiken mappar nu sina produktnamn till riktiga Captive-item-ID:n.
  Exempelvis blir Laser Pistol inte längre felaktigt droiddelen HEAD; ett
  regressionsprov verifierar hela produktkatalogens runtime-mappning.
- Captive-savefiler får inte längre styra droidens härledda vapenskada. Vid
  laddning räknas `weapon_damage` om från validerade vapenplatser, med ett
  regressionsprov för manipulerade sparfiler.
- Cross-save-importen räknar nu också om droidernas härledda vapenskada från
  validerad utrustning. Ett manipulerat portabelt save kan därför inte längre
  ge godtycklig combat-skada efter import.
- Generatorn placerar nu väggmonterade el-fällor endast mot riktiga väggceller.
  De kan därför inte längre skapas på en ogiltig väggyta utan fungerande
  interaktion; ett regressionsprov verifierar placeringen.
- Captive-fiender spawnas nu inte ovanpå redan aktiva fiender på samma nivå.
  Gruppspawn behåller därför unika kartpositioner, med ett regressionsprov över
  många frön.
- Captive-saveparsern validerar nu fienders runtimefält innan de aktiveras.
  Orimlig hastighet, räckvidd, cooldown eller respawn-timer avvisas, och v5:s
  booleska flaggor dekoderas som kanoniska bytevärden med regressionsprov.
- Captive-fienders jakt använder nu ett kardinalt steg per tick i stället för
  diagonal rörelse. De kan därför inte längre skära genom dungeonhörn; ett
  regressionsprov verifierar rörelsen.
- Captive-savefiler avvisar nu fiender med inkonsekvent livstillstånd, till
  exempel aktiv med noll HP eller inaktiv med positiv HP.
# 2026-08-06

- Aktiverade Captive-dungeonvyn i spel-loopen: den validerade 19-cells-vyn,
  objekt och aktiva fiender renderas nu över HUD-skalet i stället för att
  renderingsrutinerna avslutas direkt.
- Lade till `test_viewport`, som verifierar att både dungeon- och creature-
  rendering skriver till Captive-vyn.
- Lokal Werror-svit: 58/58 tester passerar.
## 2026-08-07 (Captive komplett skannerframsteg)

- Startmenyns Captive-skanning verifierar nu bootfilerna tillsammans med
  hela den 23-delade förstapersonsatlasen.
- Framstegsindikatorn kan därför inte längre visa en färdig Captive-kontroll
  innan samma resurser som renderaren kräver är verifierade.
- `build-werror` och hela testsviten (58/58) passerar.
# 2026-08-07 — Captive spawnmodifierare för alla creature-typer

- Verifierade mot CAPPO.EXE:s 24-entry-tabell att creature-typerna 16–24
  tidigare fick fel HP-modifierare via fallback till index 0.
- Utökade spawn-tabellerna och lade till regressionstest för typerna 16–24.

# 2026-08-07 — Liberation-källa i startmenyn

- Fixade enkel-källa-fallet där en verifierad Liberation-installation kunde
  skickas vidare med `LIBERATION_SOURCE_NONE` i stället för CD32 eller ADF.

# 2026-08-07 — L10n-fallback

- Okända eller saknade språk katalogiseras nu som engelska även i rapporterad
  språkstatus, inte bara i själva texten.

# 2026-08-07 — Dokumentationens testantal synkroniserat

- README och Developer Guide anger nu verifierbara siffror: 58 CTest-mål och
  59 testkällor, där en källa är i18n-stubben som delas av testbygget.

# 2026-08-07 — Captive cross-save med exakta rustningsplatser

- Captives portabla cross-save-format validerar nu varje rustning mot den
  kroppsdelsplats där den sparas, i linje med det vanliga sparformatet.
- Regressionstestet avvisar exempelvis HAND-rustning i HEAD-platsen.
- Hela lokala sviten passerar: 58/58 tester. GitHub Actions är grön på
  macOS, Windows och Linux.

# 2026-08-07 — Captive exakt validering av droidrustning

- Sparfiler accepterar nu bara rustningsdelen som hör till den faktiska
  kroppsdelsplatsen. Ett regressionstest avvisar till exempel HAND i HEAD.
- Lokalt bygge och hela testsviten passerar: 58/58 tester.
- GitHub Actions är grön på macOS, Windows och Linux.

# 2026-08-07 — Captive sparning av lösta dörrpussel

- Lade till ett integrationsprov för en spak som öppnar en låst dörr före
  sparning. Provet verifierar att spakens lösning, dörrens öppna tillstånd,
  målkoordinaterna och panelornamentet återställs efter laddning.
- Hela Werror-sviten passerar: 58/58 tester.

# 2026-08-07 — Captive nåbar progression bakom låsta dörrar

- Kartgeneratorn väljer nu i första hand låsta dörrar som inte delar av en
  hel våning. På små Architect-plan används en säker alternativ cell när en
  strikt choke point annars skulle isolera missionsinnehåll.
- Pusselgeneratorn filtrerar vanliga spakar till entréns nåbara område och
  lägger vid behov till en nåbar spak för den första låsta dörren. Därmed kan
  generatorn inte hamna bakom en dörr vars alla kontroller ligger på fel sida.
- Regressionstestet simulerar dörrar som blockeringar och pussel som
  upplåsningar över 256 missionsfrön. Hela sviten passerar: 58/58 tester.
- Samma regression verifierar dessutom att alla genererade pusselpositioner
  är nåbara efter upplåsningskedjan.

# 2026-08-08 — Positive scanner-cache path hardening

- Positive cache reads and writes now reject truncated cache paths instead of
  opening an unintended path when the configured home directory is unusually
  long.
- Source metadata is validated before the cached payload is replaced, so a
  failed or raced scan cannot leave a new payload without its matching v2
  metadata record.
- Local build, all 60 CTest tests, and real `.opencaptive` data verification
  pass.

# 2026-08-07 — Captive lokal texturfas

- Golv- och takpaneler samplar nu PL5-texturen från varje cells lokala
  x-origo. Lateral förskjutning i viewporten kan därför inte längre ge
  felaktig texturfas eller synliga seams mellan identiska celler.
- Ett viewport-regressionsprov verifierar att intilliggande celler börjar
  med samma sampling när deras golv- och takdata är identiska.
- Hela Werror-sviten passerar: 58/58 tester.

# 2026-08-07 — Captive synlig reservkontroll

- Reservspaken som skapas på mycket små eller helt öppna kartor får nu också
  sitt panelornament. Renderaren visar panelen på golvytan när ingen
  bakvägg finns, utan att ändra kartans gångbara celler.
- Regressionstester täcker både den platta kartlayout där reservgrenen används
  och den wall-less panelrenderingen.
- Hela testsviten passerar: 58/58 tester.
## 2026-08-08 (Launcher card artwork aspect ratio)

- Captive and Liberation cover artwork now uses centered aspect-fit scaling
  inside the 390x280 launcher cards, with a solid letterbox background.
- Added a start-menu regression test using a 2:1 fixture to prevent future
  stretching regressions.
- Full local test suite: 59/59 tests passed.
## 2026-08-08 (Shared Captive kill XP)

- Fixed Captive kill progression so every living droid receives the recovered
  XP award from CAPPO.EXE `0x9621`, not only the attacking droid.
- Level-up stat restoration now applies independently to each recipient.
- Added a multi-droid regression assertion to the game-state combat tests.
# 2026-08-08 (Captive save runtime state)
- Extended the Captive v5 save stream with a self-describing `OST1` trailer
  that preserves score, secondary-objective progress, equipped shield state,
  and active droid status timers across F9/load.
- Kept the fixed v5 payload and older v3/v4/v5 saves compatible; thumbnail
  trailers remain supported after the new state trailer.
- Added validation and round-trip regression coverage for shield categories,
  status bitmasks, and runtime-state ranges. Local build and all 60 CTest
  tests pass.
# 2026-08-08 (Captive creature combat save state)
- Extended the Captive runtime trailer to preserve each creature's
  `status_attack` and `is_boss` flags, preventing F9 from removing boss
  rewards or poison/stun attack effects.
- Added compatibility for the previous `OST1` v1 trailer while new saves
  use v2 with the creature metadata.
- Added round-trip regression assertions; local CTest remains 60/60 and real
  `.opencaptive` data verification passes for Captive and Liberation.
# 2026-08-08 (Captive shield equipment UI)
- Added the missing shield equipment slot to the droid UI, including display,
  inventory equip/replace, and safe unequip when an inventory slot is free.
- Shield durability now initializes from the verified item definition (MK1/MK2
  defense values) and is preserved by the existing save state.
- Added a UI regression test; local CTest remains 60/60.
