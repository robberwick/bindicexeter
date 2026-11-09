# bindicexeter

Exeter City Council bin collection display firmware for a Wemos/LOLIN S3 Mini (ESP32-S3FH4R2, 4 MB flash, 2 MB
PSRAM) driving a 2.9" GDEM029C90 tri-colour e-paper panel (128x296).

The device fetches the council's own bin-date data, parses the HTML fragment it returns, and draws the schedule on
the panel. There is no server, no API key, no WiFiManager portal and no OTA. Configuration lives in a LittleFS file.
Between updates the ESP32 deep-sleeps.

Data source (UPRN lookup, JSON array with an HTML `Results` fragment):

```
https://exeter.gov.uk/repositories/hidden-pages/address-finder/?qsource=UPRN&qtype=bins&term=<UPRN>
```

## Wiring

The display's MISO line is not connected, so SPI runs 3-wire with an explicit bus setup (`SPI.begin(12, -1, 11, 10)`).

| Display | S3 Mini header          |
| ------- | ----------------------- |
| BUSY    | IO4 (J2 pin 3)          |
| RST     | IO5 (J5 pin 3)          |
| DC      | IO6 (J5 pin 4)          |
| CS      | IO10 (J2 pin 7)         |
| SCK     | IO12 (J2 pin 4)         |
| MOSI    | IO11 (J2 pin 6)         |
| GND     | GND (J3 pin 6 or 7)     |
| VCC     | 3V3 (J2 pin 8)          |

Do not connect display VCC to VBUS (J3 pin 8).

Header designators follow the Wemos S3 Mini V1.0.0 schematic: J2 = EN, IO2, IO4, IO12, IO13, IO11, IO10, 3V3;
J5 = IO1, IO3, IO5, IO6, IO7, IO8, IO9, IO14; J3 = IO43, IO44, IO36, IO35, IO16, GND, GND, VBUS.

## Configuration

Copy the example and fill in your own values:

```bash
cp data/config.example.json data/config.json
```

```json
{
  "wifi_ssid": "YOUR_SSID",
  "wifi_password": "YOUR_PASSWORD",
  "uprn": "100040223370",
  "production_mode": true
}
```

`data/config.json` is git-ignored and is uploaded to LittleFS, never compiled in.

- `uprn` - the Unique Property Reference Number for the address, 10-15 digits.
- `production_mode` - `true` sleeps for 3 hours between updates; `false` sleeps 20 seconds for bench testing and adds
  a `DEV` marker to the footer.

To find a UPRN, query the address finder with the URL-encoded address:

```
https://exeter.gov.uk/repositories/hidden-pages/address-finder/?qtype=bins&term=<url-encoded-address>
```

The matching entry in the JSON array carries the `UPRN` field.

## Build and flash

```bash
pio run -e esp32-s3 -t uploadfs     # LittleFS image (data/)
pio run -e esp32-s3 -t upload       # firmware
pio device monitor -e esp32-s3
```

Run the parser tests on the host (no hardware needed):

```bash
pio test -e native
```

## Behaviour

Boot: mount LittleFS, load config, join WiFi, sync NTP, fetch the JSON, parse the `Results` fragment, draw, hibernate,
cut WiFi, deep sleep.

On the display, collections are grouped by date and headed with a relative label when the clock is known:

- `OVERDUE - Tue 10 Nov`, `TODAY - Tue 10 Nov`, `TOMORROW - Tue 10 Nov`, `IN 8 DAYS - Tue 17 Nov`
- Headers are red when the collection is today or tomorrow (or overdue), black otherwise.
- The footer shows `Updated HH:MM` (`Updated HH:MM DEV` in development mode), or `Clock not synced`.

If NTP is unavailable the device still draws the schedule, using absolute dates (`Tue 10 Nov`) and a
`Clock not synced` footer.

Sleep intervals: 3 hours in production, 20 seconds in development, 300 seconds after any error screen
(`LittleFS mount failed`, `config.json missing`, `WiFi connect failed`, `HTTP <code>`, `Bad response`,
`No collections parsed`, ...).

## Caveats

- The parser is coupled to Exeter's current `<h2>`/`<h3>` structure. If the council changes the markup the device shows
  `No collections parsed`; update `lib/schedule/ScheduleParser.cpp` and the fixture in `test/test_parser` rather than
  adding a server.
- TLS uses `setInsecure()`: the endpoint is public read-only data and, with no OTA, a pinned CA would strand the device
  after a certificate rotation.
- Garden waste only appears when the property is subscribed to the charged garden-waste service.
