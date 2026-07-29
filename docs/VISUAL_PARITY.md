# Visuell paritet

`test_visual_parity` är en pixelbaserad regressionskontroll som körs med
`ctest` och därmed i GitHub Actions. Den återger tre fasta scener och jämför
hela bildbufferten med en deterministisk 64-bitarsbildhash:

- Captives förstapersonsvy med dörr och generator
- Liberations stadsbild med fast seed (`42`) på dess egna 320×256-canvas.
  Referensbilden omfattar den CD32-inspirerade modulraden, sidopanelerna och
  det centrala stadsområdet; den blandar inte in Captives 320×200-layout.

Kontrollen fångar oavsiktliga ändringar av placering, färger, överlappning och
skalning. Referenserna är avsiktligt interna renderingsögonblick; de är inte
ett påstående om fullständig bildmässig identitet med originalutgåvorna eller
om att odetekterade grafiska resurser från speldata har återskapats.

Kör lokalt med:

```sh
ctest --test-dir build -R visual_parity --output-on-failure
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
```

Captive-fångsten kan jämföras med den hashidentifierade HUD-resursen utanför
spelvyn. Med den verifierade referensresursen är den aktuella originalramen
pixelidentisk i samtliga 47 872 pixlar utanför rektangeln `(32,55,144,112)`.
Det bevisar ram/HUD-paritet, men inte att den dynamiska förstapersonsvyn är
pixelidentisk med originalet.
