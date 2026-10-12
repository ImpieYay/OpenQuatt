# Ruwe defrostdrempels in de status-API

`GET /openquatt/odu-defrost/hp1/status` en de overeenkomstige route voor HP2
geven de reeds ingelezen einddrempels als aanvullende, alleen-lezen velden terug.
De bestaande webauthenticatie geldt. Laden gebeurt via de bestaande parameteractie;
de statusaanvraag leest zelf geen Modbus-registers.

| JSON-veld | Betekenis | Beschikbaarheid |
| --- | --- | --- |
| `exit_limit_raw` | Ruwe einddrempel | Alle ondersteunde varianten |
| `extended_exit_limit_a_raw` | Eerste aanvullende ruwe einddrempel | V1.5 en beide V2-varianten |
| `extended_exit_limit_b_raw` | Tweede aanvullende ruwe einddrempel | V1.5 en beide V2-varianten |
| `exit_counter_time_unit` | Tijdseenheid van deze tellerdrempels | Altijd `"unknown"` |

De velden bevatten een geheel getal van 0 tot en met 255, inclusief nul, of
`null` bij ontbrekende, niet ondersteunde of ongeldige data. De parameters zijn
bytewaarden in de bestaande EEPROM-interface; een niet-nulle high-byte is ongeldig.
Beschikbaarheid vereist een compleet geladen parameterprofiel en een ondersteunde
variant. Zij hangt niet af van de gekozen methode, automatische defrostvrijgave
of verse temperatuurmetingen. De waarden beschrijven de geladen instellingen;
zij bewijzen niet dat een bepaalde eindvoorwaarde momenteel actief is.

Bij communicatieverlies of identiteitswisseling wordt het profiel ongeldig.
Een status met `loaded: false` toont deze velden als `null`, ook wanneer de
diagnostieksnapshot nog oude waarden bevat. Er is geen nieuwe periodieke uitlezing,
schrijfactie, NVS-opslag of wijziging van de defrostaansturing.

De statusvelden zijn tellerdrempels, geen live tellerstanden. De fysieke tijdseenheid
en de precieze voorwaarden waaronder de buitenunit telt of reset zijn onvoldoende
bekend voor een aftelling of omrekening. `exit_required_s` blijft daarom onbekend
(`-1`); ook het onbekende mode-0-interval blijft `-1`. Een eindreden wordt niet uit
deze ruwe waarden afgeleid. De bestaande webinterface gebruikt de nieuwe velden
nog niet.

Gerichte verificatie:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -I . -I openquatt tests/host/defrost_logic_test.cpp -o /tmp/defrost_logic_test
/tmp/defrost_logic_test
g++ -std=c++17 -Wall -Wextra -Werror -I . -I openquatt tests/host/odu_defrost_status_json_test.cpp -o /tmp/odu_defrost_status_json_test
/tmp/odu_defrost_status_json_test
g++ -std=c++17 -Wall -Wextra -Werror -I . -I openquatt tests/host/oq_odu_eeprom_reference_test.cpp -o /tmp/oq_odu_eeprom_reference_test
/tmp/oq_odu_eeprom_reference_test
```

De fixtures bewaken de uitlezing en beschikbaarheid voor de vier bestaande varianten.
Hosttests bewaken daarnaast niet-geladen data, onbekende varianten, 0/255/256/65535,
het behoud van onbekende secondenwaarden en de bestaande begrensde JSON-chunks.
Deze checks vervangen geen fysieke ODU- of geheugenvalidatie onder belasting.
