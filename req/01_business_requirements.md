# 01. Бизнес-требования (`BR`)

Бизнес-требование фиксирует высокоуровневую бизнес-цель организации или заказчика системы.

| ID | Требование | Приоритет | Статус | Основание |
|---|---|---|---|---|
| BR-001 | PMM должен служить компактным персистентным storage-kernel для нижнего слоя `pjson_db` и смежных верхних слоев, не превращаясь в прикладной продукт. | Must | Recovered | `docs/pmm_target_model.md` |
| BR-002 | PMM должен позволять сохранять и восстанавливать образ управляемой памяти так, чтобы данные оставались валидными после загрузки по другому базовому адресу. | Must | Recovered | README, `docs/architecture.md` |
| BR-003 | PMM должен снижать риск повреждения персистентного образа через проверку, диагностику и восстановление служебных структур. | Must | Recovered | README, `docs/validation_model.md` |
| BR-004 | Эволюция PMM должна усиливать kernel-hardening, compaction, intrusive-index formalization и recovery/validation discipline, а не расширять прикладную семантику. | Must | Recovered | `docs/pmm_target_model.md` |
| BR-005 | PMM должен давать reusable substrate для intrusive persistent structures и persistent containers без навязывания формата данных верхнего уровня. | Should | Recovered | README, `docs/architecture.md` |
| BR-006 | PMM должен сохранять малую поверхность репозитория и API, достаточную для роли kernel, а не product/application framework. | Should | Recovered | `docs/pmm_target_model.md` |
