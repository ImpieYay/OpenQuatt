# OpenQuatt captive portal compatibility patch

ESP32-bronnen uit ESPHome **2026.10.0b3** (`esphome/components/captive_portal`),
tagcommit `2c4b1f3ed566e0b595fdb4432f3f24068402fdc3`,
onder de upstream MIT-licentie: zie `../web_server_base/LICENSE-MIT`.

Enige functionele wijzigingen: de API-key provisioningtimer sluit de portal/DNS
niet meer, de opslaan-response belooft nog geen persistent succes, en de
recovery-routes blijven buiten de actieve GET-catchall. De lokale
Wi-Fi-component sluit de portal na een geslaagde verbinding/opslag. Geen eigen
portal, credentialsysteem of AP-timer; zie `../wifi/README.md`.

The upstream component sources are identical between 2026.10.0b1 and
2026.10.0b3; the existing local patches are retained.
