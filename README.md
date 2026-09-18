# ⚔️ EMU-CMANG-TBC-BOTS

> **CMaNGOS The Burning Crusade (2.4.3)** con NPCBots mejorados, mazmorras
> scripteadas en **español latino**, addons personalizados y un sistema de
> **taberna conversacional con IA** (Qwen 3.8 vía Groq).

[![Base](https://img.shields.io/badge/base-CMaNGOS%20TBC-blue)](https://github.com/cmangos/mangos-tbc)
[![Licencia](https://img.shields.io/badge/licencia-GPL%20v2-green)](LICENSE)
[![Estado](https://img.shields.io/badge/estado-operativo-brightgreen)]()
[![Idioma](https://img.shields.io/badge/idioma-español%20latino-orange)]()

---

## 📖 Sobre este proyecto

Este repositorio es un **fork extendido de CMaNGOS TBC** (rama
`Razormaw/EMU-CMANG-TBC-BOTS`) que agrega capas de contenido y mejoras sobre
el core original, sin modificar su filosofía ni su arquitectura base.

Nació de una motivación simple: **los bugs que me frustraban como jugador,
hoy los arreglo como desarrollador.**

### ¿Qué agrega sobre CMaNGOS TBC?

| Módulo | Descripción | Estado |
|---|---|---|
| 🍺 **Canal Taberna + IA** | Canal estático global con 700+ frases en BD y conversación bot-a-bot / bot-jugador con LLM en la nube | ✅ Operativo |
| 🤖 **NPCBots mejorados** | Talentos por rama, auto-equipamiento por spec, gemas, encantamientos, roles correctos, casters a distancia | ✅ Operativo |
| 🗺️ **Mazmorras scripteadas** | Gnomeregan, Monasterio Escarlata, Zul'Farrak y La Cárcel, con textos en español latino y voces verificadas | ✅ Probado en juego |
| 🧰 **Addons personalizados** | Bagnon con filtros por calidad; pfQuest con 140+ mapas de mazmorras | ✅ Operativo |
| 🌎 **Traducciones** | Textos de jefes y SQL en español latino (política: sin formas peninsulares) | ✅ Aplicado |

---

## ⚠️ Crédito y base (léase primero)

**Todo el crédito del core, la arquitectura y el sistema de bots pertenece al
equipo de [CMaNGOS](https://github.com/cmangos)**. Ellos construyeron el
emulador, el motor de scripts y la base de NPCBots/PlayerBots.

Este proyecto **no existiría sin ellos**. Lo que aquí se agrega son capas de
contenido, scripts, traducciones y ajustes de configuración sobre su trabajo.
Si usas este repo, **respeta también la licencia y el crédito de CMaNGOS**.

- Core base: [cmangos/mangos-tbc](https://github.com/cmangos/mangos-tbc)
- Fork de bots: `Razormaw/EMU-CMANG-TBC-BOTS`

---

## ✨ Características destacadas

### 1. 🍺 Sistema "Taberna" con IA conversacional
- Canal de chat personalizado `#taberna`, creado automáticamente al arrancar
  (estático: sin dueño, sin moderadores, sin contraseña).
- **700+ frases temáticas** en base de datos (`custom_taberna_phrases`),
  editables en caliente sin recompilar (recarga cada 5 min).
- Los bots se unen solos y conversan periódicamente.
- **Conversación con LLM en la nube** (Qwen 3.8 27B vía Groq, formato
  OpenAI-compatible) con **failover automático a la BD** si la IA cae.
- **Cola prioritaria**: cuando un jugador real habla, se le responde antes
  que a las charlas bot-a-bot.
- Personaje insignia: **CervezIA, el Tabernero Inmortal** (Entry 99999,
  enano paladín tanque en Stormwind).

### 2. 🤖 NPCBots / PlayerBots mejorados
- Cambio de talentos por rama (`talents do <link>` + `reset strategies`).
- Auto-equipamiento según spec (tanque / healer / DPS melee / caster).
- Gemas en items con sockets y encantamientos habilitados
  (`minEnchantingBotLevel = 60` para TBC).
- Roles detectados por clase y rama; comportamiento correcto en grupo/raid.
- Acción `maintain ranged distance`: casters y ranged ya no se pegan al melee.
- Comandos masivos `follow` / `stay` y cola de comandos anti-spam.

### 3. 🗺️ Mazmorras completadas (scripts + español latino + voces)
| Mazmorra | Map | Jefes agregados |
|---|---|---|
| Gnomeregan | 90 | Grubbis, Viscous Fallout, Electrocutioner 6000, Crowd Pummeler 9-60, Dark Iron Ambassador |
| Monasterio Escarlata | 189 | Vishas, Thalnos, Fairbanks (+ traducción de Herod, Mograine, Whitemane, Doan, Jinete) |
| Zul'Farrak | 209 | Antu'sul, Theka, Gahz'rilla, Sezz'ziz, Ukorz (voces de trolls de Zul'Aman) |
| La Cárcel | 34 | Targorr, Kam, Hamhock, Dextren Ward, Bazil Thredd, Bruegal |

Cada jefe incluye mecánicas fieles al clásico, textos en español latino y
SoundEntries verificados contra el cliente 2.4.3.

### 4. 🧰 Addons
- **Bagnon**: filtros por calidad de item (gris→naranja) con botones de color.
- **pfQuest**: sistema `/dmap` con mapas flotantes de 140+ mazmorras
  (Classic + TBC), redimensionables y con posición guardada.

---

## 🚀 Instalación rápida

> **Requisito:** haber compilado CMaNGOS TBC según su documentación oficial.
> Este repo asume que ya sabes construir el core base.

1. Clona el repo:
   ```bash
   git clone https://github.com/Razormaw/EMU-CMANG-TBC-BOTS.git
   ```
2. Configura CMake y compila (Release x64):
   ```bash
   cmake -S . -B build
   cmake --build build --config Release
   ```
3. Ejecuta el SQL de contenido (tablas `custom_taberna_phrases`, scripts de
   mazmorras, entradas de CervezIA) sobre `tbcmangos` / `tbccharacters`.
   Los scripts están en `sql/` (ver documentación de cada módulo).
4. Ajusta `aiplayerbot.conf` (ver sección de configuración recomendada).
5. Inicia `realmd` + `mangosd`. En juego: `/join taberna`.

### Configuración recomendada (aiplayerbot.conf)
```ini
AiPlayerbot.minEnchantingBotLevel = 60     # NO usar 81 en TBC
AiPlayerbot.LLMEnabled = 3
AiPlayerbot.LLMProvider = "openai"
AiPlayerbot.LLMApiEndpoint = "https://api.groq.com/openai/v1/chat/completions"
AiPlayerbot.LLMModel = "qwen/qwen3.8-27b"
AiPlayerbot.LLMBotToBotChatChance = 25
AiPlayerbot.LLMMaxSimultaniousGenerations = 4
```

---

## 📚 Documentación

Cada módulo tiene su documento técnico con SQL completo, archivos tocados,
IDs verificados y lecciones aprendidas:

- `docs/TABERNA_IA.md` — Canal taberna, TabernaChat, CervezIA, LLM y failover.
- `docs/NPCBOTS_MEJORAS.md` — Talentos, equipamiento, roles, casters.
- `docs/MAZMORRAS.md` — Gnomeregan, Escarlata, Zul'Farrak, La Cárcel.
- `docs/ADDONS.md` — Bagnon y pfQuest.
- `docs/TRADUCCIONES.md` — Política de español latino y correcciones.

---

## 🤝 Cómo contribuir

1. Haz fork y crea una rama (`git checkout -b feature/mi-arreglo`).
2. Respeta el patrón de casteo del core: `DoCastSpellIfCan(...) == CAST_OK`.
3. Usa enums modernos (`TEMPSPAWN_*`, no `TEMPSUMMON_*`).
4. Textos nuevos: español latino, y verifica IDs de `script_texts` para no
   colisionar.
5. Documenta tu cambio en el `docs/` correspondiente.
6. Abre un Pull Request describiendo el problema y la prueba realizada.

---

## 🙏 Agradecimientos

- **Equipo CMaNGOS** — por el core, el motor de scripts y años de trabajo
  desinteresado. Este proyecto se para sobre sus hombros.
- **Comunidad de emuladores** — por los foros, wikis y herramientas
  (Wowhead TBC, HeidiSQL, QuestHelper) que hacen posible verificar cada ID.
- **Asistentes de IA** (Qwen y compañeras) — por acompañar el proceso de
  análisis, depuración y documentación. Herramientas, no autoras: el código
  y las pruebas son del desarrollador.
- **Mi familia** — por la paciencia de las noches en vela y por recordarme
  que detrás de cada bug hay un jugador que solo quiere divertirse.

---

## 📜 Licencia

Heredada de CMaNGOS: **GNU GPL v2**. Ver [LICENSE](LICENSE).
El contenido agregado (scripts, textos, traducciones) se publica bajo la
misma licencia para que cualquiera pueda usarlo, aprender y mejorar.

---

> *"Que vivan los bots de la taberna. Salud, héroes caídos."*
> — CervezIA, el Tabernero Inmortal (Entry 99999)
