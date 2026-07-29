# Visuell paritet

`test_visual_parity` är en pixelbaserad regressionskontroll som körs med
`ctest` och därmed i GitHub Actions. Den återger tre fasta scener och jämför
hela bildbufferten med en deterministisk 64-bitarsbildhash:

- Captives förstapersonsvy med dörr och generator
- Liberations stadsbild med fast seed (`42`)

Kontrollen fångar oavsiktliga ändringar av placering, färger, överlappning och
skalning. Referenserna är avsiktligt interna renderingsögonblick; de är inte
ett påstående om fullständig bildmässig identitet med originalutgåvorna.

Kör lokalt med:

```sh
ctest --test-dir build -R visual_parity --output-on-failure
```

Vid en avsiktlig visuell ändring ska den nya bilden granskas först. Uppdatera
sedan endast den berörda referenshashen och dokumentera den synliga ändringen i
samma ändring.
