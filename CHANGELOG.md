# Change Log

All notable changes will be documented in this file.

## Head

## 1.1.8 &ndash; 2026-09-04

## 1.1.7 &ndash; 2026-08-08

## 1.1.6 &ndash; 2026-07-08

### Removed

* BREAKING CHANGE: Remove default URIs (#611)

## 1.1.5 &ndash; 2026-06-06

## 1.1.4 &ndash; 2026-04-20

## 1.1.3 &ndash; 2026-03-12

## 1.1.2 &ndash; 2026-02-08

## 1.1.1 &ndash; 2025-12-14

## 1.1.0 &ndash; 2025-11-22

## 1.0.9 &ndash; 2025-09-26

## 1.0.8 &ndash; 2025-08-16

## 1.0.7 &ndash; 2025-07-02

## 1.0.6 &ndash; 2025-05-16

## 1.0.5 &ndash; 2025-03-26

## 1.0.4 &ndash; 2024-12-30

## 1.0.3 &ndash; 2024-11-26

## 1.0.2 &ndash; 2024-07-14

## 1.0.1 &ndash; 2024-04-14

## 1.0.0 &ndash; 2024-03-16

## 0.9.9 &ndash; 2024-01-28

## 0.9.8 &ndash; 2023-11-20

## 0.9.7 &ndash; 2023-09-18

## 0.9.6 &ndash; 2023-07-22

## 0.9.5 &ndash; 2023-06-12

## 0.9.4 &ndash; 2023-05-04

## 0.9.3 &ndash; 2023-03-20

## 0.9.2 &ndash; 2023-02-22

## 0.9.1 &ndash; 2023-01-12

## 0.9.0 &ndash; 2022-12-22

## 0.8.9 &ndash; 2022-11-14

### Fixed

* Unable to parse datetime (#295)

## 0.8.8 &ndash; 2022-10-04

## 0.8.7 &ndash; 2022-08-22

## 0.8.6 &ndash; 2022-07-18

## 0.8.5 &ndash; 2022-06-06

### Changed

* Market data support for `--net_disconnect_on_idle_timeout`.

## 0.8.4 &ndash; 2022-05-14

## 0.8.3 &ndash; 2022-03-22

## 0.8.2 &ndash; 2022-02-18

### Changed

* Support market data feeds without requiring account credentials (#166)

### Fixed

* Possible memory issue with JSON parsing (#181)

## 0.8.1 &ndash; 2022-01-16

## 0.8.0 &ndash; 2022-01-12

## 0.7.9 &ndash; 2021-12-08

## 0.7.8 &ndash; 2021-11-02

### Added

* Add exchange sequence number to `MarketByPrice` and `MarketByOrder` (#101)
* Add `max_trade_vol` and `trade_vol_step_size` to ReferenceData (#100)

### Changed

* Move cache utilities to API (#111)
* Interface to support binary data from web::socket
* ReferenceData currencies should follow FX conventions (#99)
* Replace `snapshot` (bool) with `update_type` (UpdateType) (#97)
* Moved signature handling to tools library (chore)

### Removed

* Remove custom literals (#110)
* Remove external rate-limiter mirroring from the REST connection (#83)

## 0.7.7 &ndash; 2021-09-20

### Changed

* Added HTTP `request_id` (#55)

## 0.7.6 &ndash; 2021-09-02

### Changed

* New order management interface (#25)

## 0.7.5 &ndash; 2021-08-08

## 0.7.4 &ndash; 2021-07-20

## 0.7.3 &ndash; 2021-07-06

## 0.7.2 &ndash; 2021-06-20

## 0.7.1 &ndash; 2021-05-30

## 0.7.0 &ndash; 2021-04-15

### Added

* Multi-account support

### Changed

* Streams to support load-balancing

## 0.6.1 &ndash; 2021-02-19

## 0.6.0 &ndash; 2021-02-02

## 0.5.0 &ndash; 2020-12-04

## 0.4.5 &ndash; 2020-11-09

## 0.4.4 &ndash; 2020-09-20

### Changed

* AssetPairs now parse the `ordermin` flag

## 0.4.3 &ndash; 2020-09-02

## 0.4.2 &ndash; 2020-07-27

### Removed

* Automake support

## 0.4.1 &ndash; 2020-07-17

## 0.4.0 &ndash; 2020-06-30

## 0.3.9 &ndash; 2020-06-09

## 0.3.8 &ndash; 2020-06-06

## 0.3.7 &ndash; 2020-05-27

## 0.3.6 &ndash; 2020-05-02

### Added

* First version supporting reference and market data

## 0.3.5 &ndash; 2020-04-22

## 0.3.4 &ndash; 2020-04-08

## 0.3.3 &ndash; 2020-03-04
