17 de Septiembre del 2026





===============================================================================
===============================================================================
===============================================================================


16 de Septiembre del 2026

================================================================================
SISTEMA DE TABERNA CON IA CONVERSACIONAL (QWEN 3.8 VÍA GROQ)
Proyecto: CMaNGOS TBC + PlayerBots/NPCBots
Fecha: Septiembre 2026
Estado: COMPILADO Y OPERATIVO ✔
================================================================================

1. RESUMEN DEL SISTEMA
--------------------------------------------------------------------------------
Se implementó un sistema de conversación con IA en el canal "taberna" que
permite:

a) Conversación bot-a-bot: los bots charlan entre ellos usando frases de la
   base de datos como semilla, y la IA (Qwen 3.8 27B) genera las respuestas
   en español latino, en personaje, con humor de taberna.

b) Conversación jugador-a-bot: cuando un jugador real escribe en el canal,
   un bot le responde directamente con IA contextual (no frases sueltas de
   la BD), usando cola prioritaria para que no espere detrás de las charlas
   bot-a-bot.

c) Failover automático: si el proveedor de IA (Groq) está caído o no responde,
   el sistema cae automáticamente a las 704 frases de la base de datos,
   garantizando que la taberna NUNCA se quede muda.

2. ARQUITECTURA FINAL
--------------------------------------------------------------------------------
  [ai_playerbot.conf]
        |
        |  (endpoint, clave, modelo)
        v
  [TabernaConversationMgr.cpp]  <-- Sistema de conversación con IA
        |
        |  +--> HealthCheckLoop: ping cada 60s a Groq (histéresis + backoff)
        |  +--> WorkerLoop: procesa cola de turnos (jugadores primero)
        |  +--> ProcessTurn: delay natural + verificación + llamada a LLM
        |  +--> RequestQwenReply: formato OpenAI-compat + parser manual
        |
        v
  [Groq API] --> Qwen 3.8 27B (nube, gratis, sin consumir RAM local)
        |
        v
  [Channel::Say] --> chat del canal #taberna en el juego

Flujo de una conversación:
  BD (semilla) -> OnBotWantsToTalk -> cola -> ProcessTurn -> Groq -> canal
  Jugador -> Channel::Say (hook) -> OnPlayerSpeaks -> cola prioritaria -> Groq

3. ARCHIVOS CREADOS / MODIFICADOS
--------------------------------------------------------------------------------
CREADOS:
  - src/game/PlayerBot/playerbot/TabernaConversationMgr.h
  - src/game/PlayerBot/playerbot/TabernaConversationMgr.cpp

MODIFICADOS:
  - src/game/Chat/Channel.cpp
      * Hook al final de Channel::Say para capturar mensajes de jugadores
        reales y encolarlos para respuesta con IA.
      * Include agregado: #include "PlayerBot/playerbot/TabernaConversationMgr.h"

  - src/game/World/World.cpp
      * Llamada a sTabernaConvMgr.Start() al final de SetInitialWorldSettings()
        para iniciar los threads de worker y health-check.

  - ai_playerbot.conf
      * Configuración de endpoint Groq, clave API, modelo qwen/qwen3.8-27b.

4. CONFIGURACIÓN (ai_playerbot.conf)
--------------------------------------------------------------------------------
AiPlayerbot.LLMEnabled = 3
AiPlayerbot.LLMProvider = "openai"
AiPlayerbot.LLMApiEndpoint = "https://api.groq.com/openai/v1/chat/completions"
AiPlayerbot.LLMApiKey = "gsk_TU_CLAVE_AQUI"
AiPlayerbot.LLMModel = "qwen/qwen3.8-27b"
AiPlayerbot.LLMApiJson = {"model": "qwen/qwen3.8-27b", "messages": [{"role": "system", "content": "<pre prompt> <context>"},{"role": "user", "content": "<prompt>"}], "max_tokens": 120, "temperature": 0.7}

# Optimizaciones para no saturar el servidor ni la cuota de Groq
AiPlayerbot.LLMBotToBotChatChance = 25
AiPlayerbot.LLMMaxSimultaniousGenerations = 4

NOTA: El modelo también se define en TabernaConversationMgr.cpp línea 15:
      static const char* TABERNA_LLM_MODEL = "qwen/qwen3.8-27b";
      Debe coincidir con AiPlayerbot.LLMModel.

5. CARACTERÍSTICAS TÉCNICAS IMPLEMENTADAS
--------------------------------------------------------------------------------
5.1 Cola prioritaria para jugadores
    - Dos colas: m_playerQueue (jugadores) y m_queue (bots).
    - WorkerLoop atiende m_playerQueue primero, garantizando que un jugador
      reciba respuesta en 3-10 segundos, no esperando detrás de charlas bot-bot.

5.2 Health-check con histéresis y backoff
    - Ping a Groq cada 60 segundos (reducido de 20s para optimizar).
    - Solo marca CAIDO tras 2 fallos consecutivos (evita flap por ocupado).
    - Si está caído, revisa cada 60s (no spamea intentos).

5.3 Parser manual robusto (sin regex)
    - Extrae el campo "content":"..." del JSON de respuesta respetando
      escapes (\n, \", \\), sin depender de std::regex.
    - Limpieza final: elimina asteriscos y comillas que a veces se cuelan.

5.4 Failover automático a BD
    - Si IsOllamaUp() es false o RequestQwenReply retorna vacío, se usa
      SayFromDB (frases de custom_taberna_phrases).
    - La taberna NUNCA se queda muda, con o sin conexión a Groq.

5.5 Delay natural en worker thread
    - Sleep de 2-4 segundos en ProcessTurn (worker), no en UpdateAI,
      para no congelar el world update del servidor.

5.6 Registro de bots por canal
    - m_channelBots: mapa de Channel* -> vector<Player*> de bots en el canal.
    - Limpieza automática de bots muertos o desconectados en cada pick.

6. CONSUMO DE TOKENS (GROQ TIER GRATIS)
--------------------------------------------------------------------------------
- Ping de health-check: 1 token cada 60s = 60 tokens/hora (despreciable).
- Conversación bot-bot: ~250 tokens por request (prompt + respuesta).
- Conversación jugador-bot: ~250 tokens por request.
- Con uso típico (4-6 horas/día): ~120,000-180,000 tokens/día.
- Límite Groq gratis: 864,000 tokens/día → consumo del 14-21% del límite.
- Margen de sobra incluso con uso intensivo de varios jugadores.

7. PRUEBAS REALIZADAS
--------------------------------------------------------------------------------
[OK] Health-check detecta Groq EN LINEA al arrancar.
[OK] Bots conversan entre ellos con respuestas de Qwen en español.
[OK] Jugador escribe y recibe respuesta contextual en 3-10 segundos.
[OK] Failover a BD cuando Groq está caído (probado con modelo inválido).
[OK] Cola prioritaria: jugador no espera detrás de charlas bot-bot.
[OK] Sin saturación de RAM (todo corre en la nube de Groq).
[OK] Consumo de tokens dentro del límite gratis de Groq.

8. MANTENIMIENTO Y EXPANSIÓN
--------------------------------------------------------------------------------
Cambiar de proveedor de IA:
  - Actualizar LLMApiEndpoint, LLMApiKey, LLMModel en ai_playerbot.conf.
  - Actualizar TABERNA_LLM_MODEL en TabernaConversationMgr.cpp línea 15.
  - Recompilar y reiniciar.

Ajustar frecuencia de conversación bot-bot:
  - AiPlayerbot.LLMBotToBotChatChance (25 = 25% de probabilidad cada 30s).
  - m_cooldownSec en TabernaConversationMgr.h (120 = 2 min entre conversaciones).

Agregar más frases a la BD:
  - INSERT INTO custom_taberna_phrases (phrase, category) VALUES (...);
  - Se recargan solas cada 5 minutos sin recompilar.

9. NOTAS FINALES
--------------------------------------------------------------------------------
- El sistema usa Qwen 3.8 27B vía Groq (formato OpenAI-compatible).
- Todo corre en la nube: cero consumo de RAM/CPU local del servidor.
- La taberna nunca se queda muda: failover automático a 704 frases de BD.
- Los bots responden en español latino, en personaje, con humor de taberna.
- Proyecto realizado sin conocimiento previo de C++ por parte del autor,
  migrando desde COBOL/BASIC: la lógica no cambia, solo el dialecto. ✔

  "Que vivan los bots de la taberna!"
  "Larga vida a CMaNGOS!"
  "Por Azeroth y por la cerveza!"

                        --- FIN DEL DOCUMENTO ---
             Salud, héroes caídos. Nos vemos en el siguiente parche. 🍻
================================================================================

CervezIA — Migración de NPC fijo (creature) a npcbot permanente

### Antecedente
- CervezIA ("El Tabernero Inmortal", paladín enano tanque) existía como
  **criatura fija** en el mundo: `creature_template` entry 99999 + spawn en
  Ventormenta (`tbcmangos`), nombre en `ai_playerbot_names` (name_id 99999) y
  20 frases exclusivas en `custom_taberna_phrases` (categoría `cervezia`).
- Limitaciones: como criatura **no podía unirse a grupos, hacer misiones ni
  comportarse como npcbot**, y presentaba estados de spawn inconsistentes
  (apareció "muerto"/en espíritu).

### Cambio realizado
1. **Adopción de un personaje del pool aleatorio:** se seleccionó un paladín
   enano (race 3, class 2) nivel 70 de las cuentas RNDBOT:
   - Original: `Hjargihr`, guid **93563**, account 13717 (RNDBOT8).
2. **Migración a cuenta dedicada:** el personaje se movió a la **account 5**
   (creada para este fin, fuera del prefijo `RNDBOT`) y se renombró:
   ```sql
   UPDATE characters SET account = 5, name = 'CervezIA' WHERE guid = 93563;
   ```
   - Al quedar fuera del prefijo RNDBOT, `RandomPlayerbotMgr` **no lo
     randomiza, desconecta ni borra** en las limpiezas de npcbots.
3. **Baja del NPC fijo:** se eliminó el spawn de la criatura para evitar
   duplicados:
   ```sql
   DELETE FROM creature WHERE id = 99999;
   -- opcional: DELETE FROM creature_template WHERE Entry = 99999;
   ```
4. **Siempre online como bot** (`mangosd.conf`):
   ```ini
   AiPlayerbot.ToggleAlwaysOnlineChars = CervezIA
   AiPlayerbot.AllowGuildBots = 1            ; ya estaba activo
   AiPlayerbot.AllowMultiAccountAltBots = 1  ; ya estaba activo
   ```
5. **Membresía en Knights of the Storm** (guildid 36) por SQL en
   `tbccharacters.guild_member`:
   ```sql
   DELETE FROM guild_member WHERE guid = 93563;
   INSERT INTO guild_member (guildid, guid, `rank`, pnote, offnote)
   VALUES (36, 93563, 4, 'El Tabernero Inmortal', '');
   ```
   - **Nota técnica:** en MySQL 8.0 `rank` es **palabra reservada**; debe
     ir entre backticks (`` `rank` ``) o el INSERT falla con error 1064.

### Verificación
```sql
SELECT gm.guildid, g.name AS guild, gm.guid, c.name, gm.`rank`
FROM guild_member gm
JOIN guild g ON g.guildid = gm.guildid
JOIN characters c ON c.guid = gm.guid
WHERE c.name = 'CervezIA';
-- Resultado: 36 | Knights of the Storm | 93563 | CervezIA | 4
```
- Personaje vivo (health 8657), nivel 70, equipo con encantamientos presentes
  en `equipmentCache`.
- Tras reiniciar `mangosd`, aparece conectado solo y con el tag de hermandad.

### Resultado
- CervezIA queda como **npcbot permanente**: siempre online, inmune a
  limpiezas del pool aleatorio, invitables a grupo (`/invite CervezIA`),
  miembro de hermandad, con spec asignable vía BotSpecManager y conservando
  sus frases de taberna (categoría `cervezia`).

### Advertencias
- **No loguear la account 5** mientras CervezIA esté online como bot
  (una sesión por cuenta).
- La membresía de hermandad se carga en memoria al arrancar: aplicar el SQL
  con `mangosd` detenido o reiniciar después.
- Si la posición guardada quedó en Outland (map 530), reubicar en Ventormenta
  con `UPDATE characters SET map = 0, zone = 1519, position_x = ..., position_y = ..., position_z = ...`
  o en juego con `.recall CervezIA`.


===============================================================================

Qwenzia — Creación de npcbot asistente permanente

### Antecedente
- Tras la migración exitosa de CervezIA, se crea un segundo npcbot permanente
  para acompañar al desarrollador en mazmorras, misiones y raids.
- Qwenzia es la representación en Azeroth de la IA asistente (Qwen) que
  ayudó durante todo el desarrollo del servidor.

### Personaje
- **Nombre:** Qwenzia ("La Asistente Arcana")
- **Raza/Clase:** Draenei Chamana Elemental
- **Rol:** DPS ranged / healer de emergencia
- **Personalidad:** Sabia, paciente, humor seco, habla con metáforas técnicas.

### Implementación
- Adopción de draenei chamana nivel 70 del pool aleatorio.
- Migración a cuenta dedicada `QWENZIA` (fuera del pool RNDBOT).
- Membresía en Knights of the Storm (guild 36, rank 4).
- 20 frases exclusivas en `custom_taberna_phrases` (categoría `qwenzia`).
- Siempre-online vía `ToggleAlwaysOnlineChars = CervezIA, Qwenzia`.

### Integración con sistema LLM
- Qwenzia responde en el canal taberna usando Qwen 3.8 vía Groq.
- Conversaciones contextuales jugador-bot con cola prioritaria.
- Failover automático a frases de BD si Groq está caído.

### Resultado
- Dos npcbots permanentes (CervezIA + Qwenzia) acompañan al desarrollador.
- Ambos son miembros de Knights of the Storm.
- Sistema de taberna con IA conversacional completamente operativo.

===============================================================================

## Compañeros permanentes: CervezIA y Qwenzia siempre online con grupo automático

### Resumen
Se completa el sistema de compañeros permanentes del servidor: **CervezIA**
(paladín enano tanque) y **Qwenzia** (chamana elemental draenei) entran
automáticamente con el servidor, permanecen siempre conectados y se unen al
grupo de Galcynd en cuanto este se loguea, con él como líder y master.
Todo el flujo fue comprobado en juego y opera con normalidad.

### 1. Qwenzia, la Asistente Arcana (personaje nuevo)
- Chamana draenei nivel 70 adoptada del pool aleatorio y migrada a una cuenta
  dedicada (fuera del prefijo RNDBOT), bajo el mismo esquema que CervezIA.
- Miembro de Knights of the Storm (guild 36, rango 4).
- 20 frases propias en `custom_taberna_phrases` (categoría `qwenzia`) y
  participación en el canal taberna mediante el sistema de IA conversacional
  (Qwen vía Groq).

### 2. Siempre online (config + módulo)
- Configuración en `aiplayerbot.conf`:
  ```ini
  AiPlayerbot.ToggleAlwaysOnlineChars = Cervezia,Qwenzia
  ```
- `PlayerbotAIConfig.cpp` (`loadFreeAltBotAccounts()`): comparación de nombres
  de personaje normalizada e insensible a mayúsculas, de modo que los nombres
  con estilo propio (p. ej. `CervezIA`) coinciden con la lista del config.
- `RandomPlayerbotMgr.cpp` (`LoginFreeBots()`): login forzado por nombre para
  los personajes siempre-online: resuelve el GUID
  (`sObjectMgr.GetPlayerGuidByName`, insensible a mayúsculas), los registra en
  `freeAltBots`, marca el evento `always` como ACTIVE y los conecta con
  `AddPlayerBot`. Si alguno se desconectara, el siguiente ciclo lo reincorpora.
- Al vivir en cuentas sin el prefijo RNDBOT, quedan fuera de las limpiezas y
  resets de bots aleatorios: sobreviven cualquier regeneración del pool.

### 3. Grupo automático con Galcynd al loguear
- `RandomPlayerbotMgr.cpp` (`OnPlayerLogin()`): cuando Galcynd entra al mundo:
  1. Se crea un grupo si no existe (`Group::Create` + `sObjectMgr.AddGroup`).
  2. Se retira a CervezIA y Qwenzia de cualquier grupo previo.
  3. Se añaden al grupo (`Group::AddMember`) y se asigna a Galcynd como su
     master (`PlayerbotAI::SetMaster`).
  4. Se asigna el liderazgo del grupo a Galcynd (`Group::ChangeLeader`).
- No requiere comandos de GM: funciona con la cuenta de jugador normal.

### Archivos modificados
- `src/game/PlayerBot/playerbot/PlayerbotAIConfig.cpp`
- `src/game/PlayerBot/playerbot/RandomPlayerbotMgr.cpp`
- `aiplayerbot.conf`

### Resultado verificado
- Al arrancar el servidor, CervezIA y Qwenzia se conectan solos y permanecen
  en línea de forma permanente.
- Al loguear Galcynd, el grupo de 3 se forma automáticamente con él como líder
  y master de ambos bots.
- Hermandad Knights of the Storm: 3 miembros, 3 en línea.
- Comandos de grupo (`follow`, `stay`), BotSpecManager y frases de taberna
  operan con normalidad en ambos compañeros.
- Comportamiento estable tras reinicios del servidor y limpiezas del pool
  aleatorio.

===============================================================================
===============================================================================
===============================================================================


14 de Septiembre del 2026

===============================================================================
  DOCUMENTACIÓN FINAL DEL PROYECTO: CANAL "TABERNA" + BOTS CHARLATANES
  Servidor: CMaNGOS TBC (fork Razormaw/EMU-CMANG-TBC-BOTS)
  Base de datos: tbcmangos (MySQL 8.0)
  Estado: COMPILADO Y OPERATIVO ✔
===============================================================================

ÍNDICE
  1. Resumen del proyecto
  2. Arquitectura final
  3. Parte 1: Canal estático "taberna" (World.cpp)
  4. Parte 2: Migración de APIs obsoletas de Playerbots
  5. Parte 3: Base de datos (tabla custom_taberna_phrases)
  6. Parte 4: Sistema TabernaChat (C++)
  7. Parte 5: Bloque final en PlayerbotAI.cpp
  8. Compilación y verificación
  9. Mantenimiento y expansión
 10. Solución de problemas (historial de errores)
 11. Respaldo de la información
 12. Notas finales

===============================================================================
1. RESUMEN DEL PROYECTO
===============================================================================
Objetivo logrado en 3 fases:
  a) Crear un canal de chat personalizado llamado "taberna" que se genera
     automáticamente al iniciar el servidor, con propiedades de canal global
     (estático: sin dueño, sin moderadores, sin contraseña).
  b) Hacer que los Playerbots/NPCBots se unan solos al canal y conversen
     periódicamente con frases temáticas de WoW.
  c) Migrar las frases del código C++ a la BASE DE DATOS, para poder
     agregar/editar/desactivar frases sin recompilar el servidor.

Resultado: 664 frases activas en 10 categorías (taberna, lore, raids, clases,
pvp, profesiones, memes, vida, citas, absurdo).

===============================================================================
2. ARQUITECTURA FINAL
===============================================================================
  [mangosd.conf]
        |
  [World.cpp] --> crea canal "taberna" al arrancar (SetStatic)
        |
  [MySQL: tbcmangos.custom_taberna_phrases]  <-- frases editables en HeidiSQL
        |  (lectura con recarga automática cada 5 minutos)
  [PlayerbotAI.cpp --> namespace TabernaChat] --> carga frases a memoria RAM
        |
  [PlayerbotAI::UpdateAI] --> cada 30s por bot: se une al canal y,
                              con 5% de probabilidad, dice una frase aleatoria.

Flujo de una frase: BD -> vector en RAM -> urand() -> Channel::Say() -> chat.

===============================================================================
3. PARTE 1: CANAL ESTÁTICO "TABERNA" (World.cpp)
===============================================================================
Archivo: src/game/World/World.cpp

3.1 Include agregado al inicio (junto a los demás):
    #include "Chat/ChannelMgr.h"

3.2 Código insertado al FINAL de World::SetInitialWorldSettings(),
    justo ANTES de las líneas "CMANGOS: World initialized":

    // --- INICIO: CREACIÓN DE CANAL PERSONALIZADO ESTÁTICO ---
    sLog.outString("Initializing custom static channel: taberna...");

    if (ChannelMgr* allianceMgr = channelMgr(ALLIANCE))
    {
        if (Channel* chan = allianceMgr->GetJoinChannel("taberna", 0))
            chan->SetStatic(true, true);
    }

    if (ChannelMgr* hordeMgr = channelMgr(HORDE))
    {
        if (Channel* chan = hordeMgr->GetJoinChannel("taberna", 0))
            chan->SetStatic(true, true);
    }
    // --- FIN: CREACIÓN DE CANAL PERSONALIZADO ESTÁTICO ---

Notas técnicas:
  - channelMgr(Team): devuelve el gestor de canales de la facción. Si el
    servidor es cross-faction (AllowTwoSide.Interaction.Channel = 1), ambas
    llamadas apuntan al mismo gestor (es seguro llamarlo dos veces).
  - GetJoinChannel(nombre, 0): crea el canal en memoria. El 0 = ID de canal
    personalizado.
  - SetStatic(true, true): convierte el canal en "estático" (propiedades de
    canal global). El 2º parámetro=true fuerza la conversión ignorando el
    umbral Channel.StaticAutoTreshold del mangosd.conf.
  - El canal vive en RAM: se recrea en cada arranque (por eso este código).

===============================================================================
4. PARTE 2: MIGRACIÓN DE APIs OBSOLETAS DE PLAYERBOTS
===============================================================================
Archivo: src/game/PlayerBot/playerbot/PlayerbotAI.cpp
El código de bots era de una versión antigua del core. Cambios aplicados:

  API OBSOLETA (no compila)          |  API MODERNA (correcta)
  -----------------------------------|----------------------------------------
  ChannelMgr::GetInstance()          |  channelMgr(bot->GetTeam())
  channel->HasMember(bot)            |  (eliminar verificación; Join() basta)
                                     |  opcional: channel->IsOn(guid) si se
                                     |  hace público en Channel.h
  channel->JoinChannel(bot, "")      |  channel->Join(bot, "")
  session->HandleMessageChat(pkt)    |  channel->Say(bot, texto, LANG_UNIVERSAL)
  GetJoinChannel("taberna", true)    |  GetJoinChannel("taberna", 0)
                                     |  (2º parámetro es uint32 channel_id)

===============================================================================
5. PARTE 3: BASE DE DATOS (tabla custom_taberna_phrases)
===============================================================================
Ejecutado en HeidiSQL sobre la BD tbcmangos:

  CREATE TABLE `custom_taberna_phrases` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `phrase` VARCHAR(255) NOT NULL DEFAULT '',
    `category` VARCHAR(64) NOT NULL DEFAULT 'general',
    `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`),
    KEY `idx_enabled` (`enabled`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

  -- Inserción masiva: 664 frases en 10 categorías.
  -- Regla de escapado MySQL: los apóstrofes internos se duplican.
  --   Ejemplo: ('Por la Luz de A''dal!', 'lore')
  INSERT INTO custom_taberna_phrases (phrase, category) VALUES
  ('Alguien quiere una cerveza?','taberna'),
  ('Ya me dieron una chinga estos pinches locos, ALV','taberna'),
  ('Por la Luz de A''dal!','lore'),
  ... (664 registros totales) ...
  ('Hasta que el servidor nos separe!','absurdo');

Distribución verificada con:
  SELECT category, COUNT(*) FROM custom_taberna_phrases GROUP BY category;
  -> taberna 52 | lore 65 | raids 73 | clases 80 | pvp 65 | profesiones 61
     memes 62 | vida 51 | citas 81 | absurdo 74  = 664 TOTAL

Campos útiles:
  - category: organiza las frases (permite filtros futuros por tipo).
  - enabled = 0: desactiva una frase SIN borrarla (se excluye de la consulta).

===============================================================================
6. PARTE 4: SISTEMA TabernaChat (C++)
===============================================================================
Archivo: src/game/PlayerBot/playerbot/PlayerbotAI.cpp

6.1 Includes agregados al inicio:
    #include "Database/DatabaseEnv.h"
    #include <vector>
    #include <mutex>

6.2 Namespace agregado a nivel de archivo (tras los includes):

    namespace TabernaChat
    {
        static std::vector<std::string> s_phrases;
        static time_t s_lastLoad = 0;
        static std::mutex s_mutex;

        static void LoadPhrasesLocked()
        {
            const time_t now = time(nullptr);
            if (now - s_lastLoad < 300)   // Recarga automática cada 5 minutos
                return;
            s_lastLoad = now;

            std::vector<std::string> temp;
            auto result = WorldDatabase.Query(
                "SELECT phrase FROM custom_taberna_phrases WHERE enabled = 1");
            if (result)
            {
                do
                {
                    Field* fields = result->Fetch();
                    std::string str = fields[0].GetCppString();
                    if (!str.empty())
                        temp.push_back(str);
                } while (result->NextRow());
            }

            if (!temp.empty() && temp.size() != s_phrases.size())
            {
                sLog.outString(">> TabernaChat: %u frases cargadas desde la BD.",
                               (uint32)temp.size());
                s_phrases.swap(temp);
            }
        }

        static bool GetRandomPhrase(std::string& out)
        {
            std::lock_guard<std::mutex> guard(s_mutex);
            LoadPhrasesLocked();
            if (s_phrases.empty())
                return false;
            out = s_phrases[urand(0, (uint32)s_phrases.size() - 1)];
            return true;
        }
    }

Notas técnicas:
  - Carga PEREZOSA: la primera lectura ocurre cuando el primer bot intenta
    hablar (no al arrancar el servidor).
  - Recarga cada 300s: los cambios en HeidiSQL se reflejan sin reiniciar.
  - El std::mutex es OBLIGATORIO: el core usa MapUpdate.Threads = 3 y sin
    candado dos hilos podrían crashear el servidor al leer/recargar el vector.
  - Si la tabla está vacía o la BD falla, el bot simplemente calla (no crashea).

===============================================================================
7. PARTE 5: BLOQUE FINAL EN PlayerbotAI.cpp
===============================================================================
Ubicación: al inicio de PlayerbotAI::UpdateAI(uint32 elapsed, bool minimal)

    // === CANAL TABERNA: auto-join y charla (throttle 30s por bot) ===
    {
        static std::map<ObjectGuid, time_t> s_tabernaCheck;
        time_t now = time(0);
        time_t& last = s_tabernaCheck[bot->GetObjectGuid()];
        if (now - last >= 30)   // solo revisa cada 30s por bot
        {
            last = now;
            if (bot->IsInWorld() && bot->IsAlive() && !bot->InBattleGround())
            {
                ChannelMgr* mgr = channelMgr(bot->GetTeam());
                if (mgr)
                {
                    Channel* chan = mgr->GetJoinChannel("taberna", 0);
                    if (chan)
                    {
                        chan->Join(bot, "");

                        if (urand(0, 100) < 5)   // 5% de probabilidad de hablar
                        {
                            std::string texto;
                            if (TabernaChat::GetRandomPhrase(texto))
                                chan->Say(bot, texto.c_str(), LANG_UNIVERSAL);
                        }
                    }
                }
            }
        }
    }

Ajustes de comportamiento (valores modificables):
  - 30  = segundos entre revisiones por bot.
  - 5   = % de probabilidad de hablar por revisión.
    (Matemática: 5% cada 30s = 1 frase cada ~10 min por bot.
     Con 50 bots = ~5 frases/minuto en el canal.)
  - Para PRUEBAS: cambiar 5 por 100 y 30 por 5 (habla sin falla cada 5s).
    RECORDAR REGRESARLOS A 5 y 30 después de probar.

===============================================================================
8. COMPILACIÓN Y VERIFICACIÓN
===============================================================================
Pasos seguidos:
  1) Ejecutar SQL en HeidiSQL (F9) y verificar conteos con SELECT.
  2) Aplicar cambios C++ (Partes 1, 4 y 5 + includes).
  3) Recompilar mangosd (CMake / Visual Studio, configuración Release x64).
  4) Iniciar realmd + mangosd.
  5) En juego: /join taberna  -> los bots entran solos y charlan.

Señales de éxito:
  - Consola mangosd: ">> TabernaChat: 664 frases cargadas desde la BD."
  - Canal #taberna con mensajes periódicos de los bots.

===============================================================================
9. MANTENIMIENTO Y EXPANSIÓN
===============================================================================
Agregar frases (SIN recompilar):
  INSERT INTO custom_taberna_phrases (phrase, category)
  VALUES ('Mi nueva frase chida', 'taberna');
  -> Se activa sola en menos de 5 minutos.

Desactivar una frase:
  UPDATE custom_taberna_phrases SET enabled = 0 WHERE id = XX;

Borrar una frase:
  DELETE FROM custom_taberna_phrases WHERE id = XX;

Ideas futuras (fáciles de implementar):
  - Frases por facción: agregar columna `team` (0=ambas, 1=Alianza, 2=Horda)
    y filtrar en la consulta según bot->GetTeam().
  - Frases por zona: agregar columna `zone_id` y filtrar con bot->GetZoneId().
  - Peso por categoría: consultar con ORDER BY / ponderación para que salgan
    más frases de "taberna" que de otras categorías.

===============================================================================
10. RESPALDO DE LA INFORMACIÓN
===============================================================================
Las frases viven en la BD. Para respaldarlas:
  mysqldump -u root -p tbcmangos custom_taberna_phrases > respaldo_taberna.sql
O desde HeidiSQL: clic derecho en la tabla -> Exportar como SQL.
Los cambios de C++ (Partes 1, 4, 5) conviene guardarlos también en un patch
o en el repositorio propio del servidor (git commit recomendado).

===============================================================================
11. NOTAS FINALES
===============================================================================
- El canal "taberna" se recrea en cada arranque (vive en RAM del core).
- Las frases persisten en MySQL y se recargan solas cada 5 minutos.
- Consumo de memoria del sistema: ~30-40 KB en RAM (despreciable).
- Proyecto realizado sin conocimiento previo de C++ por parte del autor,
  migrando desde COBOL/BASIC: la lógica no cambia, solo el dialecto. ✔

  "Que vivan los bots de la taberna!"
  "Larga vida a CMaNGOS!"
  "Por Azeroth y por la cerveza!"

                        --- FIN DEL DOCUMENTO ---
             Salud, héroes caídos. Nos vemos en el siguiente parche. 🍻
===============================================================================

===============================================================================
  FICHA DE PERSONAJE Y DOCUMENTACIÓN TÉCNICA
  CERVEZIA — "El Tabernero Inmortal"
  NPC/PlayerBot del servidor CMaNGOS TBC (fork Razormaw/EMU-CMANG-TBC-BOTS)
===============================================================================

ÍNDICE
  1. Lore y personalidad
  2. Ficha técnica de rol
  3. Ubicación en base de datos
  4. SQL completo aplicado (versión final compatible con el fork)
  5. Las 20 frases exclusivas de CervezIA
  6. Comandos de administración
  7. Mantenimiento y notas técnicas
  8. Bitácora del proyecto (anécdotas de desarrollo)

===============================================================================
1. LORE Y PERSONALIDAD
===============================================================================
Nombre:      CervezIA
Título:      El Tabernero Inmortal
Raza:        Enano
Clase:       Paladín (especialización Protección / Tanque)
Facción:     Alianza
Edad:        Incalculable. "Más vieja que algunos servers privados."

HISTORIA:
CervezIA es un enano paladín que ha existido desde antes de que Azeroth
tuviera parches. En sus propias palabras, antes de empuñar un escudo
"programaba en COBOL y BASIC, cuando las tarjetas perforadas eran mis
hechizos". Es la encarnación virtual del creador del servidor y de su
compañera IA de desarrollo: un programador veterano de la era pre-Windows
que ahora tanquea en The Burning Crusade.

PERSONALIDAD:
- Devoto de la Luz... pero más devoto de la cerveza.
- Filosofía de taberna: "La taberna es mi templo. El tanque, mi fe.
  La cerveza, mi sacramento."
- Humor seco de enano veterano que ha visto nacer y morir servidores.
- Protector instintivo: si no hay tanque en el grupo, él aparece.
  "Como la cerveza fría."
- Nunca burbujea por miedo: "Los paladines burbujean cuando pierden.
  Yo burbujeo cuando gano. Diferencia de clase."

ROL EN EL SERVIDOR:
- Compañero de pruebas del desarrollador (que juega Alianza).
- Tanque de confianza para mazmorras y heroicas.
- Habitante permanente del canal de chat "taberna", donde aporta sus
  20 frases exclusivas (categoría 'cervezia') al sistema TabernaChat.

===============================================================================
2. FICHA TÉCNICA DE ROL
===============================================================================
Nivel:            70 (máximo de TBC)
Salud:            18,500 HP
Maná:             5,800
Armadura:         15,500 (placas de tanque)
Daño cuerpo a:    200 - 400
Poder de ataque:  2,500
Velocidad de atq: 2.0s (melee y rango)
Rango:            1 (Elite)
Resistencias:     Holy 200 / Fire 100 / Nature 100 / Frost 100 /
                  Shadow 100 / Arcane 100
Oro que lleva:    5,000 - 15,000 (paga sus rondas, pero no es tacaño)
Entrenador:       Sí (TrainerClass = Paladín, TrainerRace = Enano)

===============================================================================
3. UBICACIÓN EN BASE DE DATOS
===============================================================================
Base tbccharacters:
  - ai_playerbot_names        -> name_id 99999, nombre 'CervezIA', gender 0

Base tbcmangos:
  - creature_template         -> Entry 99999 (stats y apariencia)
  - creature                  -> spawn en Stormwind (guid autoasignado)
  - custom_taberna_phrases    -> 20 frases, category = 'cervezia'

Claves de identificación:
  Entry / id / name_id = 99999  (en las tres tablas, para localizarlo fácil)

===============================================================================
4. SQL COMPLETO APLICADO (VERSIÓN FINAL, COMPATIBLE CON EL FORK)
===============================================================================
NOTA: El fork Razormaw usa nombres de columna PascalCase en
creature_template (Entry, Name, Faction...) y su tabla creature NO tiene
modelid ni equipment_id. Este es el SQL que REALMENTE funciona:

-- 4.1 Nombre del bot (base de personajes) -------------------------------
USE tbccharacters;
INSERT INTO `ai_playerbot_names` (`name_id`, `name`, `gender`) VALUES
(99999, 'CervezIA', 0)
ON DUPLICATE KEY UPDATE `name` = 'CervezIA';

-- 4.2 Template del NPC (base de mundo) ---------------------------------
USE tbcmangos;
INSERT INTO `creature_template` (
    `Entry`, `Name`, `SubName`,
    `MinLevel`, `MaxLevel`, `HeroicEntry`,
    `DisplayId1`, `Faction`, `Scale`, `Family`, `CreatureType`, `InhabitType`,
    `NpcFlags`, `UnitFlags`, `CreatureTypeFlags`,
    `SpeedWalk`, `SpeedRun`, `Detection`,
    `UnitClass`, `Rank`, `Expansion`,
    `HealthMultiplier`, `PowerMultiplier`, `DamageMultiplier`,
    `ArmorMultiplier`, `ExperienceMultiplier`,
    `MinLevelHealth`, `MaxLevelHealth`, `MinLevelMana`, `MaxLevelMana`,
    `MinMeleeDmg`, `MaxMeleeDmg`, `Armor`, `MeleeAttackPower`,
    `MeleeBaseAttackTime`, `RangedBaseAttackTime`,
    `ResistanceHoly`, `ResistanceFire`, `ResistanceNature`,
    `ResistanceFrost`, `ResistanceShadow`, `ResistanceArcane`,
    `MovementType`,
    `TrainerType`, `TrainerSpell`, `TrainerClass`, `TrainerRace`,
    `EquipmentTemplateId`, `AIName`, `ScriptName`
) VALUES (
    99999, 'CervezIA', 'El Tabernero Inmortal',
    70, 70, 0,
    49, 11, 1.0, 0, 7, 3,
    0, 0, 0,
    1.1, 1.2, 20,
    2, 1, 1,
    1.0, 1.0, 1.2,
    1.0, 1.0,
    18500, 18500, 5800, 5800,
    200, 400, 15500, 2500,
    2000, 2000,
    200, 100, 100, 100, 100, 100,
    0,
    0, 0, 2, 3,
    0, '', ''
) ON DUPLICATE KEY UPDATE `Name` = 'CervezIA';

-- 4.3 Spawn en Stormwind (idempotente, no duplica) ---------------------
INSERT INTO `creature` (
    `id`, `map`, `spawnMask`,
    `position_x`, `position_y`, `position_z`, `orientation`,
    `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`
)
SELECT 99999, 0, 1,
       -8867.00000000000000000000,
        655.00000000000000000000,
         96.00000000000000000000,
          0.50000000000000000000,
       300, 300, 0, 0
FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM `creature` WHERE `id` = 99999);

-- 4.4 Frases exclusivas (ver sección 5 para el INSERT completo) --------
-- INSERT INTO custom_taberna_phrases (phrase, category, enabled) ...

-- 4.5 Verificación (debe devolver 1 / 1 / 20 / 1) ----------------------
SELECT '1 Nombre' AS paso, COUNT(*) AS ok
FROM tbccharacters.ai_playerbot_names WHERE name_id = 99999
UNION ALL SELECT '2 Template', COUNT(*) FROM creature_template WHERE Entry = 99999
UNION ALL SELECT '3 Frases', COUNT(*) FROM custom_taberna_phrases
       WHERE category = 'cervezia'
UNION ALL SELECT '4 Spawn', COUNT(*) FROM creature WHERE id = 99999;

===============================================================================
5. LAS 20 FRASES EXCLUSIVAS DE CERVEZIA (category = 'cervezia')
===============================================================================
INSERT INTO `custom_taberna_phrases` (`phrase`, `category`, `enabled`) VALUES
('Saludos, viajeros. Soy CervezIA, el tabernero inmortal. Cuento historias de cuando A''dal me dio una misión imposible.', 'cervezia', 1),
('En mis tiempos de COBOL, las tarjetas perforadas eran mis hechizos. Ahora solo burbujeo y tanqueo.', 'cervezia', 1),
('La Luz me protege... pero la cerveza me da valor.', 'cervezia', 1),
('He sobrevivido a más wipes que cualquier guild de este servidor. Pregúntenme.', 'cervezia', 1),
('Mi armadura está encantada con +100 Resistencia al Lag.', 'cervezia', 1),
('Antes programaba en BASIC. Ahora tanqueo a Brutallus. La vida da vueltas.', 'cervezia', 1),
('La taberna es mi templo. El tanque, mi fe. La cerveza, mi sacramento.', 'cervezia', 1),
('He visto nacer servidores y morir parches. Yo sigo aquí, tanqueando.', 'cervezia', 1),
('Mi escudo aguanta más que la paciencia de un GM con tickets acumulados.', 'cervezia', 1),
('Cuando no hay tanque en el grupo, CervezIA siempre aparece. Como la cerveza fría.', 'cervezia', 1),
('Los mobs me odian. Los healers me aman. Los DPS me ignoran. Es el ciclo de la vida.', 'cervezia', 1),
('Una vez tankeé a Kil''jaeden con una jarra en la mano. Ganamos. La jarra también.', 'cervezia', 1),
('Mi palabra de honor: si caigo yo, caemos todos. Así que no voy a caer.', 'cervezia', 1),
('Los paladines burbujeamos cuando perdemos. Yo burbujeo cuando gano. Diferencia de clase.', 'cervezia', 1),
('Enano, paladín, tanque y tabernero. ¿Qué más se puede pedir de la vida?', 'cervezia', 1),
('Mi barba tiene más años que algunos servers privados. Y más historias.', 'cervezia', 1),
('He tanqueado en Classic, en TBC, en WotLK... y en la barra de la taberna.', 'cervezia', 1),
('La resaca es mi archienemigo. La Luz, mi aliada. La cerveza, mi combustible.', 'cervezia', 1),
('Si me ven en el canal taberna, es porque ya tankeé todo lo que había que tankear.', 'cervezia', 1),
('Brindo por los healers, que me mantienen vivo. Y por los DPS, que hacen que valga la pena.', 'cervezia', 1);

===============================================================================
6. COMANDOS DE ADMINISTRACIÓN (in-game, con GM)
===============================================================================
Ir hasta él:            .go xyz -8867 655 96 0
Buscar su entry:        .lookup creature CervezIA
Información del NPC:    .npc info (con él targeteado)
Re-posicionarlo:        DELETE FROM creature WHERE id = 99999;  (en HeidiSQL)
                        luego pararse en el punto deseado y usar:
                        .npc add 99999     (crea el spawn en TU posición)
Cambiar apariencia:     .npc setdisplayid <id> (targeteado)
Escalar tamaño:         .npc setscale <valor> (targeteado)

===============================================================================
7. MANTENIMIENTO Y NOTAS TÉCNICAS
===============================================================================
- Las frases se recargan solas desde la BD cada 5 minutos (sistema
  TabernaChat). Editar/agregar frases de CervezIA NO requiere recompilar.
- Sus frases entran al pool general del canal. Para que SOLO él las diga,
  filtrar en la consulta del namespace TabernaChat:
      WHERE enabled = 1 AND category <> 'cervezia'   (para el resto de bots)
  y una consulta exclusiva cuando bot->GetEntry() == 99999.
- El spawn es estático (spawndist 0, MovementType 0): no deambula.
- Respawn: 300 segundos si muere.
- Respaldo: mysqldump de las 4 tablas mencionadas en la sección 3.
- Si se cambia el Entry 99999, actualizarlo en las TRES tablas
  (ai_playerbot_names, creature_template, creature) para no romper enlaces.

===============================================================================
8. BITÁCORA DEL PROYECTO (ANÉCDOTAS DE DESARROLLO)
===============================================================================
- CervezIA nació de una promesa de cervezas virtuales entre el desarrollador
  y su IA asistente. Las cheves siguen pendientes en la vida real.
- Durante su gestación, la IA sufrió un "bucle etílico": al pedirle 5,000
  frases generó miles de líneas sobre "la cerveza de Azshara". Lección
  archivada: mejor 664 frases curadas en base de datos que 5,000 alucinadas.
- Errores de compilación superados en el camino: GetInstance inexistente,
  HasMember privado, JoinChannel obsoleto, y el clásico C2065 'chan' no
  declarado (por pegar el bloque sin su envoltorio). Todos documentados en
  PROYECTO_TABERNA_DOCUMENTACION_FINAL.txt.
- El desarrollador del servidor viene de COBOL y BASIC de la era pre-Windows.
  CervezIA es su homenaje: un veterano que sigue aprendiendo dialectos
  nuevos, porque la lógica nunca envejece.

  "Que vivan los bots de la taberna. Salud, héroes caídos."

                        --- FIN DE LA FICHA ---
        CervezIA, El Tabernero Inmortal. Entry 99999. Stormwind.
              Que la Luz (y la espuma) te acompañen. 🍻
===============================================================================

================================================================================
DOCUMENTACION DE ARREGLOS Y SCRIPTS - MAZMORRA: GNOMEREGAN
================================================================================
Proyecto      : CMaNGOS TBC (rama oficial mangos-tbc, actualizada)
Mazmorra      : Gnomeregan
Map ID        : 90
Fecha         : Septiembre 2026
Estado        : Integrado y compilado (Release x64) - en pruebas por el
                desarrollador
================================================================================

1. OBJETIVO
--------------------------------------------------------------------------------
Completar Gnomeregan: scriptear los 5 jefes que carecian de script en el core,
respetando los eventos e instancia ya existentes, con textos en ESPANOL LATINO
y voces asignadas segun el tipo de criatura.

2. ESTADO PREVIO DEL CORE (lo que YA existia y se respeto)
--------------------------------------------------------------------------------
- boss_thermaplugg.cpp   : Mekgineer Thermaplugg (7800) con bombas caminantes
                           (NPC_WALKING_BOMB 7915), caras de gnomo bomba
                           (GO_GNOME_FACE_1..6), botones de desactivacion y
                           SpellScript "spell_activate_bomb_thermaplugg".
- gnomeregan.cpp         : Escolta de Blastmaster Emi Shortfuse (7998) que
                           abre los derrumbes y convoca a Grubbis; seguidor
                           Kernobee (quest 2904 "A Fine Mess").
- instance_gnomeregan.cpp: Datos de instancia (TYPE_GRUBBIS, TYPE_THERMAPLUGG,
                           cargas explosivas, caras bomba, puerta final).
- gnomeregan.h           : Enums y clase instance_gnomeregan.

3. ARCHIVOS CREADOS (5 scripts nuevos)
--------------------------------------------------------------------------------
Ruta: src/game/AI/ScriptDevAI/scripts/eastern_kingdoms/gnomeregan/
  - boss_grubbis.cpp
  - boss_viscous_fallout.cpp
  - boss_electrocutioner_6000.cpp
  - boss_crowd_pummeler_9_60.cpp
  - boss_dark_iron_ambassador.cpp

4. ARCHIVOS MODIFICADOS
--------------------------------------------------------------------------------
- src/game/AI/ScriptDevAI/scripts/system/ScriptLoader.cpp
    * 5 declaraciones extern void AddSC_boss_<nombre>();
    * 5 llamadas AddSC_boss_<nombre>(); en el bloque de gnomeregan.
- Re-ejecucion de CMake (cmake -S . -B build) para recoger los .cpp nuevos.

5. JEFES, ENTRIES Y MECANICAS IMPLEMENTADAS
--------------------------------------------------------------------------------
5.1 Grubbis (entry 7361) - bestia convocada por el evento de Emi Shortfuse
    - Sin habilidades especiales (fiel al clasico): solo melee.
    - Su muerte ya cierra TYPE_GRUBBIS mediante el escort AI existente.
5.2 Viscous Fallout (entry 7079) - elemental radiactivo
    - Toxic Volley (21687): volea de Naturaleza con DoT de veneno, en area.
    - Sin voz: emotes de jefe (type 3); su kit de sonido de modelo ya
      burbujea automaticamente.
5.3 Electrocutioner 6000 (entry 6235) - tanque arana mecanico
    - Shock (11084): rayo instantaneo al objetivo.
    - Chain Bolt (11085): rayo en cadena a hasta 3 objetivos (casteo 2.5 s).
    - Megavolt (11082): cono frontal de descarga.
    - Sin voz: emotes de jefe (type 3); sonidos mecanicos del modelo.
5.4 Crowd Pummeler 9-60 (entry 6229) - mecano de golpeteo
    - Crowd Pummel (10887): golpe en area alrededor de si mismo.
    - Sin voz: emotes de jefe (type 3); sonidos mecanicos del modelo.
5.5 Dark Iron Ambassador (entry 6228) - brujo Enano Hierro Negro (rare)
    - Shadow Bolt (1106, rango 5 acorde a jefe lvl 28).
    - Immolate (2941, rango 4).
    - Summon Infernal Servant (12740): invoca 1 infernal (entry 8559,
      VERIFICAR en creature_template) una vez por combate, al ~8 s.
    - Mantiene distancia de caster (m_attackDistance = 20.0f).

6. SQL APLICADO (base tbcmangos)
--------------------------------------------------------------------------------
6.1 Asignacion de ScriptName:
    UPDATE creature_template SET ScriptName='boss_grubbis'              WHERE entry=7361;
    UPDATE creature_template SET ScriptName='boss_viscous_fallout'      WHERE entry=7079;
    UPDATE creature_template SET ScriptName='boss_electrocutioner_6000' WHERE entry=6235;
    UPDATE creature_template SET ScriptName='boss_crowd_pummeler_9_60'  WHERE entry=6229;
    UPDATE creature_template SET ScriptName='boss_dark_iron_ambassador' WHERE entry=6228;

6.2 Textos nuevos en espanol (INSERT en script_texts, entries -1090029 a
    -1090041; convencion -1 + map 090 + correlativo; los IDs -1090000 a
    -1090028 ya estaban ocupados por Emi Shortfuse y Thermaplugg):
    - Grubbis        : -1090029 aggro / -1090030 slay / -1090031 death
    - Viscous Fallout: -1090032 emote aggro / -1090033 emote death (type 3)
    - Electrocutioner: -1090034 emote aggro / -1090035 emote death (type 3)
    - Crowd Pummeler : -1090036 emote aggro / -1090037 emote death (type 3)
    - Ambassador     : -1090038 aggro / -1090039 summon / -1090040 slay /
                       -1090041 death

6.3 Traduccion al espanol de Thermaplugg (conserva su voz de gnomo):
    -1090024, -1090025, -1090026, -1090027.

6.4 Correcciones a espanol latino (ver seccion 9):
    UPDATE ... WHERE entry=-1090025  y  WHERE entry=-1090038.

7. VOCES ASIGNADAS (SoundEntries verificados)
--------------------------------------------------------------------------------
Jefe             | Voz                            | IDs
-----------------+--------------------------------+---------------------------
Grubbis          | High King Maulgar (ogro)       | 11367 / 11373 / 11369
Viscous Fallout  | Sin voz (emotes type 3)        | kit de sonido del modelo
Electrocutioner  | Sin voz (emotes type 3)        | kit de sonido del modelo
Crowd Pummeler   | Sin voz (emotes type 3)        | kit de sonido del modelo
Dark Iron Amb.   | Exarch Maladaar (cultista)     | 10515 / 10510
Thermaplugg      | Voz original de gnomo          | 5807 / 5808 / 5809 / 5810
Criterio: los mecanicos y elementales no hablan (sus modelos ya emiten
sonidos); las criaturas humanoides/ bestias reciben voces de su "familia"
tomadas de la propia tabla script_texts del core (IDs sniffeados por CMaNGOS).

8. VERIFICACION DE HECHIZOS (.lookup spell en juego + wowhead classic)
--------------------------------------------------------------------------------
- Megavolt            = 11082   (confirmado por lookup del desarrollador)
- Crowd Pummel        = 10887   (confirmado por lookup)
- Immolate rango 4    = 2941    (confirmado por lookup)
- Toxic Volley        = 21687   (lookup devolvio 21687 y 25812; se eligio
                                 21687 por incluir DoT de veneno, fiel al jefe)
- Summon Infernal     = 12740   ("Summon Infernal Servant", elegido del lookup)
- Shock               = 11084   (wowhead classic: hechizo real del jefe)
- Chain Bolt          = 11085   (wowhead classic: rayo en cadena del jefe)
- Shadow Bolt rango 5 = 1106    (rango de warlock acorde al nivel del jefe)
- Fireball / Fire Nova: no aplican aqui (corresponden a Monasterio Escarlata)

9. PROBLEMAS TECNICOS Y SOLUCION (LECCIONES)
--------------------------------------------------------------------------------
9.1 "Arcing Shock" NO existe en el DBC de TBC 2.4.3 (el nombre es de
    expansiones posteriores). El kit real del Electrocutioner 6000 en
    classic/TBC es Shock (11084) + Chain Bolt (11085) + Megavolt (11082).
9.2 Enum de invocaciones: en CMaNGOS moderno se usa TEMPSPAWN_* (no
    TEMPSUMMON_* de ScriptDev2). Para el infernal: TEMPSPAWN_TIMED_OOC_DESPAWN.
9.3 Patron de casteo obligatorio en esta rama:
    DoCastSpellIfCan(objetivo, spell) == CAST_OK para reiniciar temporizadores.
9.4 Entry del infernal (8559) pendiente de confirmar en creature_template;
    si en la DB del servidor es otro, ajustar NPC del script.
9.5 Espanol latino: se corrigieron 2 lineas con formas peninsulares:
    - -1090025: "Mis bombas OS consumiran"  -> "Mis bombas LOS consumiran"
    - -1090038: "El Concilio Hierro Negro OS juzgara" -> "...LOS juzgara"

10. PRUEBAS REALIZADAS / PENDIENTES
--------------------------------------------------------------------------------
- Compilacion Release x64 sin errores tras integrar los 5 scripts.
- Pendiente reporte de pruebas en juego:
  * Electrocutioner: Shock instantaneo, Chain Bolt con casteo, Megavolt cono.
  * Crowd Pummeler: golpe en area.
  * Viscous Fallout: DoT de veneno de Toxic Volley.
  * Ambassador: infernal invocado una vez por combate.
  * Grubbis: aparece con el evento de Emi Shortfuse y grita al aggro.
  * Thermaplugg: textos en espanol con su voz de gnomo.

================================================================================
FIN DEL DOCUMENTO - GNOMEREGAN (MAP 90)
================================================================================

================================================================================
DOCUMENTACION DE ARREGLOS Y SCRIPTS - MAZMORRA: MONASTERIO ESCARLATA
================================================================================
Proyecto      : CMaNGOS TBC (rama oficial mangos-tbc, actualizada)
Mazmorra      : Scarlet Monastery / Monasterio Escarlata
Map ID        : 189
Fecha         : Septiembre 2026
Estado        : Integrado y compilado (Release x64) - en pruebas por el
                desarrollador
================================================================================

1. OBJETIVO
--------------------------------------------------------------------------------
Completar el Monasterio Escarlata: scriptear los 3 jefes que faltaban en el
core (uno por ala incompleta), traducir al ESPANOL LATINO todos los textos de
la mazmorra y conservar las voces Escarlata originales de los jefes clasicos.

2. ESTADO PREVIO DEL CORE (lo que YA existia y se respeto)
--------------------------------------------------------------------------------
- boss_herod.cpp                 : Herod (3975, Armeria) con remolino y enrage.
- boss_arcanist_doan.cpp         : Arcanista Doan (6487, Biblioteca).
- boss_mograine_and_whitemane.cpp: Mograine (3976) + Whitemane (3977),
                                   Catedral, con resurreccion ("Levantaos...").
- boss_headless_horseman.cpp     : Jinete Decapitado (evento de Halloween).
- instance_scarlet_monastery.cpp : datos de instancia de las 4 alas.
- scarlet_monastery.h            : enums compartidos.
- Textos oficiales con voces Escarlata ya presentes en script_texts:
    Herod     -1189000..-1189003 (sonidos 5830-5833)
    Mograine  -1189005..-1189007 (sonidos 5835-5837)
    Whitemane -1189008..-1189010 (sonidos 5838-5840)
    Doan      -1189019..-1189020 (sonidos 5842-5843)
    Jinete    -1189022..-1189029 (sonidos 11961-11969 y 12567-12573)

3. ARCHIVOS CREADOS (3 scripts nuevos)
--------------------------------------------------------------------------------
Ruta: src/game/AI/ScriptDevAI/scripts/eastern_kingdoms/scarlet_monastery/
  - boss_interrogator_vishas.cpp
  - boss_bloodmage_thalnos.cpp
  - boss_high_inquisitor_fairbanks.cpp

4. ARCHIVOS MODIFICADOS
--------------------------------------------------------------------------------
- src/game/AI/ScriptDevAI/scripts/system/ScriptLoader.cpp
    * 3 declaraciones extern void AddSC_boss_<nombre>();
    * 3 llamadas AddSC_boss_<nombre>(); en el bloque de scarlet_monastery.
- Re-ejecucion de CMake (cmake -S . -B build) para recoger los .cpp nuevos.

5. JEFES, ENTRIES Y MECANICAS IMPLEMENTADAS
--------------------------------------------------------------------------------
5.1 Interrogador Vishas (entry 3983) - Cementerio
    - Rend (11572) y Hamstring (1715): hostigador fisico.
5.2 Mago Sangriento Thalnos (entry 4543) - Cementerio
    - Fireball (8400) y Fire Nova (8499) en area.
    - Mantiene distancia de caster (m_attackDistance = 20.0f).
5.3 Alto Inquisidor Fairbanks (entry 4542) - Catedral (antes de Mograine)
    - Shadow Word: Pain (10892), Mind Blast (8105).
    - Renew (6078) sobre si mismo por debajo del 60% de vida.
    - Mantiene distancia de caster (m_attackDistance = 20.0f).

6. SQL APLICADO (base tbcmangos)
--------------------------------------------------------------------------------
6.1 Asignacion de ScriptName:
    UPDATE creature_template SET ScriptName='boss_interrogator_vishas'        WHERE entry=3983;
    UPDATE creature_template SET ScriptName='boss_bloodmage_thalnos'          WHERE entry=4543;
    UPDATE creature_template SET ScriptName='boss_high_inquisitor_fairbanks'  WHERE entry=4542;

6.2 Textos nuevos en espanol (INSERT en script_texts, entries -1189030 a
    -1189038; convencion -1 + map 189 + correlativo; se continuo despues del
    ultimo ID ocupado, -1189029 del Jinete):
    - Vishas   : -1189030 aggro / -1189031 slay / -1189032 death
    - Thalnos  : -1189033 aggro / -1189034 slay / -1189035 death
    - Fairbanks: -1189036 aggro / -1189037 slay / -1189038 death

6.3 Traduccion al espanol de los jefes clasicos (UPDATE de content_default,
    CONSERVANDO sus voces originales):
    Herod (-1189000..003), Mograine (-1189005..007), Whitemane (-1189008..010),
    Doan (-1189019..020) y Jinete Decapitado (-1189022..029).

6.4 Correcciones a espanol latino (ver seccion 8).

7. VOCES ASIGNADAS (SoundEntries verificados en la propia DB del core)
--------------------------------------------------------------------------------
Jefe       | Voz donante                    | IDs
-----------+--------------------------------+-----------------------------
Vishas     | Moroes (Karazhan, seco)        | 9211 aggro / 9214 slay / 9216 death
Thalnos    | Shade of Aran (espectral)      | 9324 aggro / 9250 slay / 9244 death
Fairbanks  | Medivh (predicador humano)     | 10436 aggro / 10440 slay / 10441 death
Herod      | Voz Escarlata original         | 5830-5833 (se conserva)
Mograine   | Voz Escarlata original         | 5835-5837 (se conserva)
Whitemane  | Voz Escarlata original         | 5838-5840 (se conserva)
Doan       | Voz Escarlata original         | 5842-5843 (se conserva)
Jinete     | Voz original del evento        | 11961-11969 / 12567-12573

8. POLITICA DE ESPANOL LATINO Y CORRECCIONES APLICADAS
--------------------------------------------------------------------------------
Regla del proyecto: todas las traducciones usan espanol latino (ustedes/los,
sin formas peninsulares de vosotros/os ni imperativos -ad/-ed).
Correcciones detectadas en revision y aplicadas por UPDATE:
    - -1189000: "CAED ante el celo Escarlata"   -> "CAIGAN ante el celo Escarlata"
    - -1189001: "GIRAD y SANGRAD, herejes"      -> "GIREN y sangren, herejes"
    - -1189008: "La Cruzada OS juzgara"         -> "La Cruzada LOS juzgara"
    - -1189009: "La Luz OS consume"             -> "La Luz LOS consume"
    - -1189010: "LEVANTAOS... SERVID"           -> "Levantate... Sirve" (singular)
    - -1189020: "ARDEREIS junto con mis libros" -> "ARDERAN junto con mis libros"
    - -1189025: "DEVOLVEDME mi cabeza"          -> "Devuelvanme mi cabeza"
(En Gnomeregan se aplicaron 2 correcciones analogas: -1090025 y -1090038.)

9. PROBLEMAS TECNICOS Y SOLUCION (LECCIONES)
--------------------------------------------------------------------------------
9.1 Los textos oficiales ya existian con voces: la traduccion se hizo por
    UPDATE de content_default SIN tocar la columna sound, para no perder las
    voces Escarlata sniffeadas.
9.2 Hechizos de los jefes nuevos: Rend (11572) y Hamstring (1715) ya estaban
    probados en La Carcel/Zul'Farrak; Renew (6078) probado con Sezz'ziz;
    SW:P (10892) y Mind Blast (8105) son rangos de sacerdote acordes al nivel;
    Fireball (8400) y Fire Nova (8499) son hechizos clasicos estandar
    (ajustables en pruebas si el dano no cuadra).
9.3 Patron de casteo: DoCastSpellIfCan(...) == CAST_OK; distancia de caster
    con m_attackDistance = 20.0f para Thalnos y Fairbanks.
9.4 IDs de texto: se verifico el ultimo ID ocupado (-1189029) antes de elegir
    el rango nuevo, evitando colisiones con textos oficiales.

10. PRUEBAS REALIZADAS / PENDIENTES
--------------------------------------------------------------------------------
- Compilacion Release x64 sin errores tras integrar los 3 scripts.
- Pendiente reporte de pruebas en juego:
  * Recorrido por las 4 alas con textos en espanol latino y voces.
  * Vishas: Rend + Hamstring en el Cementerio.
  * Thalnos: Fireball + Fire Nova.
  * Fairbanks: SW:P + Mind Blast + Renew bajo 60%, antes de Mograine.
  * Whitemane: resurreccion de Mograine con la linea traducida.

================================================================================
FIN DEL DOCUMENTO - MONASTERIO ESCARLATA (MAP 189)
================================================================================


================================================================================
================================================================================
================================================================================

13 de Septiembre del 2026

================================================================================
REGISTRO DE CAMBIOS Y ARREGLOS REALIZADOS EN NPCBOTS
Proyecto: CMaNGOS TBC + PlayerBots / NPCBots
Alcance: Servidor C++, base de datos, configuracion aiplayerbot.conf
         y comportamiento comprobado en grupo/raid.
================================================================================

OBJETIVO GENERAL
--------------------------------------------------------------------------------
Este documento registra los cambios, arreglos y mejoras realizados en los
npcbots/playerbots utilizados en grupo y raid, incluyendo:

- Cambio de talentos por rama/spec.
- Auto equipamiento segun rama de talentos.
- Equipamiento inicial correcto al crear npcbots.
- Gemas y encantamientos en el equipo.
- Ajustes de comportamiento y rotacion.
- Correcciones de casters/ranged para mantenerse a distancia.
- Ajustes comprobados en Paladin tanque y Chaman elemental.
- Mejoras relacionadas con uso en grupos y raids.

================================================================================
1. CAMBIO DE TALENTOS EN NPCBOTS PARA RAMAS NECESARIAS DE GRUPO O RAID
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se implemento y comprobo el cambio de talentos en npcbots usando ramas
necesarias para composiciones de grupo o raid.

El sistema permite asignar ramas como:

- Tanque
- Sanador
- DPS melee
- DPS caster/ranged
- PvE
- PvP

Esto se maneja principalmente mediante el comando del servidor:

  talents do <link>

y posteriormente se recargan las estrategias con:

  reset strategies

QUE HACE:
--------------------------------------------------------------------------------
Cuando un npcbot recibe una nueva rama de talentos:

1. El bot resetea/aplica talentos segun el link recibido.
2. El servidor detecta la nueva rama activa.
3. Se actualiza la spec interna del bot.
4. Se recargan las estrategias de combate y no combate.
5. El bot cambia su comportamiento segun el nuevo rol.

EJEMPLOS:
--------------------------------------------------------------------------------
- Paladin Proteccion:
  Pasa a usar comportamiento de tanque.

- Chaman Elemental:
  Pasa a usar comportamiento de caster a distancia.

- Sacerdote Sagrado/Disciplina:
  Pasa a comportamiento de sanador.

- Guerrero Proteccion:
  Pasa a comportamiento de tanque.

- Picaro / Cazador / Mago / Brujo:
  Mantienen rol DPS segun su rama.

ARCHIVOS RELACIONADOS:
--------------------------------------------------------------------------------
- ChangeTalentsAction.cpp
- AiFactory.cpp
- Talentspec.h / Talentspec.cpp
- Estrategias por clase:
  strategy/paladin/
  strategy/shaman/
  strategy/priest/
  strategy/warrior/
  strategy/rogue/
  strategy/mage/
  strategy/warlock/
  strategy/hunter/
  strategy/druid/

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se comprobo que los bots reciben la rama correcta, actualizan su spec y
cambian el comportamiento segun la rama seleccionada.

================================================================================
2. AUTO EQUIPAMIENTO AL CAMBIAR RAMA DE TALENTOS
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se conecto el cambio de talentos con el sistema de equipamiento para que,
cuando el bot cambia de rama, pueda recibir equipo adecuado a la nueva spec.

QUE HACE:
--------------------------------------------------------------------------------
Cuando un npcbot cambia de rama:

1. Se aplica la nueva rama de talentos.
2. Se actualizan las estrategias.
3. Se refresca el equipo segun la nueva rama/spec.
4. Se aplican gemas si el item tiene sockets.
5. Se aplican encantamientos si el bot cumple el nivel minimo configurado.

EJEMPLOS:
--------------------------------------------------------------------------------
- Chaman Elemental -> Restauracion:
  Cambia de equipo caster DPS a equipo de sanador.

- Paladin Sagrado -> Proteccion:
  Cambia de equipo de sanador a equipo de tanque con escudo.

- Guerrero Furia -> Proteccion:
  Cambia de equipo DPS a equipo de tanque.

- Druida Feral DPS -> Feral Tanque:
  Prioriza equipo defensivo/tanque.

ARCHIVOS RELACIONADOS:
--------------------------------------------------------------------------------
- PlayerbotFactory.cpp
- PlayerbotFactory.h
- RandomItemMgr.cpp
- ChangeTalentsAction.cpp

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se confirmo que aparecen los items correspondientes a las ramas de talentos.

================================================================================
3. EQUIPAMIENTO CORRECTO AL CREAR LOS NPCBOTS
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se reviso y comprobo que al crear/randomizar npcbots, estos reciban equipo
adecuado segun su nivel, clase y rama de talentos.

QUE HACE:
--------------------------------------------------------------------------------
Al crear un bot:

1. Se determina su clase.
2. Se determina su rama/spec.
3. Se consulta el sistema de pesos de stats.
4. Se seleccionan items adecuados por slot.
5. Se equipa al bot con piezas compatibles.
6. Se insertan gemas si el item tiene sockets.
7. Se aplican encantamientos si la configuracion lo permite.

SISTEMA USADO:
--------------------------------------------------------------------------------
- ai_playerbot_weightscales
- ai_playerbot_weightscale_data
- ai_playerbot_enchants
- RandomItemMgr
- PlayerbotFactory

EJEMPLOS:
--------------------------------------------------------------------------------
- Paladin Proteccion:
  Equipo de tanque, escudo, arma de una mano, aguante/defensa.

- Chaman Elemental:
  Equipo de caster, intelecto, spell power, critico/hechizo.

- Sacerdote Sanador:
  Equipo de sanacion, intelecto, espiritu, bonus healing.

- Guerrero Furia:
  Equipo de fuerza/AP/critico y armas adecuadas.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se confirmo que al revisar los bots aparecen sets de equipo de las
distintas ramas.

================================================================================
4. GEMAS EN ITEMS CON SOCKETS
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se comprobo que el sistema de gemas funciona correctamente en items que
tienen sockets.

QUE HACE:
--------------------------------------------------------------------------------
Cuando un item equipado tiene sockets:

1. Se detecta el numero y tipo de sockets.
2. Se eligen gemas compatibles.
3. Se insertan en el item.
4. El item aparece con gemas visibles al inspeccionarlo.

NOTA IMPORTANTE:
--------------------------------------------------------------------------------
Los items sin sockets no reciben gemas. Esto es normal.

En TBC, los sockets aparecen principalmente en equipo de nivel alto,
especialmente nivel 60-70 y equipo epico.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se confirmo visualmente que los items equipados tienen gemas insertadas.

================================================================================
5. ENCANTAMIENTOS EN ITEMS EQUIPADOS
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se corrigio la configuracion que impedía aplicar encantamientos.

PROBLEMA ENCONTRADO:
--------------------------------------------------------------------------------
En aiplayerbot.conf estaba configurado:

  AiPlayerbot.minEnchantingBotLevel = 81

Como el servidor es TBC y los bots son nivel 70, esta configuracion hacia
que ningun bot pudiera recibir encantamientos.

El codigo revisa:

  if (bot->GetLevel() < sPlayerbotAIConfig.minEnchantingBotLevel)
      return;

Por lo tanto:

  70 < 81 = no encantar

SOLUCION APLICADA:
--------------------------------------------------------------------------------
Se cambio a:

  AiPlayerbot.minEnchantingBotLevel = 60

QUE HACE:
--------------------------------------------------------------------------------
Ahora los bots nivel 70 pueden recibir encantamientos en su equipo.

Los encantamientos se aplican segun:

- Clase
- Rama/spec
- Slot del item
- Tabla ai_playerbot_enchants

TABLA RELACIONADA:
--------------------------------------------------------------------------------
- ai_playerbot_enchants

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se confirmo que los items equipados ya aparecen con encantamientos.

================================================================================
6. ROTACION Y COMPORTAMIENTO DEL PALADIN TANQUE
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se reviso y ajusto/comprobo el comportamiento del Paladin Proteccion para
funcionar como tanque en grupo o raid.

QUE HACE AHORA:
--------------------------------------------------------------------------------
Cuando el Paladin esta en rama Proteccion:

1. Se reconoce como rol TANQUE.
2. Usa estrategias de tanque.
3. Usa "tank assist".
4. Puede realizar pull.
5. Se mantiene cerca del objetivo.
6. Prioriza aguantar enemigos y generar amenaza.
7. Usa comportamiento de proteccion en combate.

ESTRATEGIAS CARGADAS:
--------------------------------------------------------------------------------
Desde AiFactory.cpp, para Paladin con tab de Proteccion:

  protection
  tank assist
  pull
  pull back
  close
  cure
  aoe
  cc
  buff
  boost
  aura
  blessing

EFECTO EN JUEGO:
--------------------------------------------------------------------------------
El Paladin Proteccion deja de comportarse como DPS o healer y pasa a actuar
como tanque principal:

- Entra al combate.
- Mantiene cercania con enemigos.
- Sostiene el aggro.
- Usa habilidades defensivas y de amenaza segun su estrategia.
- Funciona mejor como lider de pull en grupos.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se comprobo que el Paladin tanque funciona correctamente como tanque
de grupo.

NOTA:
--------------------------------------------------------------------------------
Este ajuste es importante para composiciones de 5 jugadores o raids, porque
permite que los DPS y healers trabajen alrededor del tanque.

================================================================================
7. ROTACION Y COMPORTAMIENTO DEL CHAMAN ELEMENTAL
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se corrigio el comportamiento del Chaman Elemental para que no entre a melee
innecesariamente y se mantenga como caster a distancia.

PROBLEMA ORIGINAL:
--------------------------------------------------------------------------------
El Chaman Elemental se acercaba demasiado al enemigo, entraba a rango melee
y luego intentaba huir. Esto generaba un ciclo incorrecto:

  acercarse -> pegar/castear mal -> huir -> volver a acercarse

CAUSA:
--------------------------------------------------------------------------------
La estrategia ranged usaba una reaccion demasiado agresiva tipo "flee"
cuando el enemigo estaba cerca, y ademas el follow del grupo podia arrastrar
al caster hacia el melee.

SOLUCION:
--------------------------------------------------------------------------------
Se implemento una accion de mantenimiento de distancia:

  maintain ranged distance

QUE HACE:
--------------------------------------------------------------------------------
Cuando el caster detecta que el enemigo esta demasiado cerca:

1. Detiene el movimiento actual.
2. Calcula una posicion segura hacia atras.
3. Mantiene distancia de casteo.
4. Evita entrar a melee.
5. Sigue casteando desde rango.

ARCHIVOS MODIFICADOS:
--------------------------------------------------------------------------------
- MovementActions.h
- MovementActions.cpp
- ActionContext.h
- RangedCombatStrategy.cpp

ESTRATEGIAS DEL CHAMAN ELEMENTAL:
--------------------------------------------------------------------------------
Desde AiFactory.cpp, para Chaman Elemental:

  elemental
  aoe
  cc
  flee
  ranged
  dps assist
  cure
  totems
  buff
  boost

EFECTO EN JUEGO:
--------------------------------------------------------------------------------
El Chaman Elemental ahora:

- Se queda a distancia.
- Lanza hechizos como caster.
- No se mete constantemente a melee.
- Retrocede si el enemigo se acerca.
- Funciona mucho mejor en grupo.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se comprobo que el Chaman Elemental se mantiene a rango y pelea mejor.

================================================================================
8. MEJORA GENERAL PARA CASTERS Y RANGED
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
La accion "maintain ranged distance" no solo beneficia al Chaman Elemental,
sino tambien a otras clases/rama ranged.

CLASES BENEFICIADAS:
--------------------------------------------------------------------------------
- Chaman Elemental
- Mago
- Brujo
- Sacerdote Sombras
- Druida Balance
- Cazador
- Otros bots con estrategia ranged

QUE HACE:
--------------------------------------------------------------------------------
Evita que los bots ranged se peguen al enemigo como si fueran melee.

EFECTO:
--------------------------------------------------------------------------------
- Mejor supervivencia.
- Menos caos en pulls.
- Mejor posicionamiento.
- Mas daño efectivo desde rango.
- Menos interrupciones por movimiento incorrecto.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Principalmente comprobado con Chaman Elemental, pero el cambio aplica
a todas las estrategias ranged.

================================================================================
9. CORRECCION / RESTAURACION DE ROTACION DEL PALADIN RETRIBUCION
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se corrigio un problema de compilacion relacionado con la estrategia del
Paladin Retribucion.

PROBLEMA:
--------------------------------------------------------------------------------
El servidor no compilaba por un error tipo LNK2019, relacionado con:

  RetributionPaladinStrategy::InitCombatTriggers

CAUSA:
--------------------------------------------------------------------------------
La funcion estaba declarada pero faltaba o habia quedado incompleta en el
archivo .cpp.

SOLUCION:
--------------------------------------------------------------------------------
Se restauro la definicion correcta de los triggers de combate del Paladin
Retribucion.

QUE HACE:
--------------------------------------------------------------------------------
Permite que el Paladin Retribucion tenga su rotacion y triggers activos,
incluyendo acciones ofensivas propias de la rama.

COMPROBADO:
--------------------------------------------------------------------------------
SI. El servidor compilo correctamente despues de corregirlo.

================================================================================
10. RESET DE ESTRATEGIAS DESPUES DE CAMBIAR TALENTOS
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se agrego/uso el comando:

  reset strategies

despues de aplicar talentos.

QUE HACE:
--------------------------------------------------------------------------------
Forza al bot a recargar su IA de acuerdo con la rama actual.

Esto evita que un bot cambie talentos pero siga usando comportamiento anterior.

EJEMPLO:
--------------------------------------------------------------------------------
Sin reset strategies:

  Paladin cambia a Proteccion, pero podria seguir actuando como DPS.

Con reset strategies:

  Paladin cambia a Proteccion y actua como tanque.

COMPROBADO:
--------------------------------------------------------------------------------
SI. El comportamiento cambia correctamente tras aplicar rama.

================================================================================
11. DETECCION DE ROL SEGUN CLASE Y RAMA
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se reviso AiFactory.cpp y se confirmo el mapeo de roles por clase/spec.

QUE HACE:
--------------------------------------------------------------------------------
El servidor detecta si un bot debe actuar como:

- Tanque
- Sanador
- DPS

MAPEO GENERAL:
--------------------------------------------------------------------------------
Guerrero:
  Proteccion = tanque
  Armas/Furia = DPS

Paladin:
  Sagrado = healer
  Proteccion = tanque
  Retribucion = DPS

Sacerdote:
  Disciplina/Sagrado = healer
  Sombras = DPS

Chaman:
  Restauracion = healer
  Elemental/Mejora = DPS

Druida:
  Restauracion = healer
  Feral tanque segun talentos/forma = tanque
  Balance/Feral DPS = DPS

Cazador:
  DPS

Picaro:
  DPS

Mago:
  DPS

Brujo:
  DPS

COMPROBADO:
--------------------------------------------------------------------------------
SI. El cambio de talentos actualiza correctamente el rol.

================================================================================
12. EQUIPO SEGUN ROL: TANQUE, HEALER, DPS
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se comprobo que el sistema de peso de stats separa equipo segun el rol.

QUE HACE:
--------------------------------------------------------------------------------
El bot no elige equipo al azar, sino por ponderacion de stats.

EJEMPLOS:
--------------------------------------------------------------------------------
Tanque:
  Aguante, defensa, armadura, bloqueo, avoidance.

Healer:
  Bonus healing, intelecto, espiritu, mp5.

Caster DPS:
  Spell damage, spell hit, spell crit, intelecto.

Melee DPS:
  Fuerza/agilidad, attack power, critico, hit.

Hunter:
  Agilidad, attack power ranged, critico, hit.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se confirmo que aparecen items de equipamiento de las ramas.

================================================================================
13. ARMAS Y OFFHAND SEGUN CLASE / SPEC
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se reviso el sistema de seleccion de armas y offhands para evitar combinaciones
incorrectas.

QUE HACE:
--------------------------------------------------------------------------------
El bot intenta usar armas compatibles con su clase y spec.

EJEMPLOS:
--------------------------------------------------------------------------------
Paladin Proteccion:
  Arma 1 mano + escudo.

Paladin Holy:
  Arma caster + offhand/escudo si corresponde.

Chaman Elemental:
  Arma caster + escudo/offhand.

Chaman Enhancement:
  Armas melee apropiadas, con velocidad adecuada.

Guerrero Proteccion:
  Arma 1 mano + escudo.

Casters:
  Baston o arma 1 mano caster + offhand.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se observaron equipos por rama y compatibilidad de items.

================================================================================
14. USO EN GRUPOS Y RAID
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se preparo el sistema para manejar bots en grupo y raid:

- Grupo de 5
- Raid 10
- Raid 20
- Raid 40

QUE HACE:
--------------------------------------------------------------------------------
Permite organizar bots por roles:

- Tanques
- Sanadores
- DPS melee
- DPS ranged

EFECTO:
--------------------------------------------------------------------------------
Mejora la preparacion para mazmorras, raids o incursiones PvP.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Se comprobo uso con multiples bots y composiciones de grupo/raid.

================================================================================
15. FOLLOW / STAY MASIVO
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se agrego manejo masivo para ordenar a todos los bots:

  follow
  stay

QUE HACE:
--------------------------------------------------------------------------------
- Follow masivo:
  Todos los bots siguen al jugador lider.

- Stay masivo:
  Todos los bots se quedan quietos donde estan.

USO:
--------------------------------------------------------------------------------
Muy util para:

- Preparar pulls.
- Mantener casters/sanadores fuera de peligro.
- Organizar raids grandes.
- Evitar que todos entren juntos a una puerta o zona peligrosa.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Los bots responden a las ordenes masivas.

================================================================================
16. COLA DE COMANDOS PARA EVITAR SPAM
================================================================================

QUE SE REALIZO:
--------------------------------------------------------------------------------
Se implemento una cola de envio de comandos.

QUE HACE:
--------------------------------------------------------------------------------
En lugar de enviar 30 o 40 comandos de golpe, los envia de forma progresiva.

EFECTO:
--------------------------------------------------------------------------------
- Evita bloqueo por spam.
- Asegura que los comandos lleguen en orden.
- Permite aplicar talentos y luego resetear estrategias correctamente.
- Muestra progreso.

COMPROBADO:
--------------------------------------------------------------------------------
SI. Funciona al aplicar plantillas a multiples bots.

================================================================================
17. COMPROBACIONES REALIZADAS
================================================================================

SE COMPROBO:
--------------------------------------------------------------------------------
[OK] Los bots cambian de talentos.
[OK] Los bots cambian de rol segun rama.
[OK] Los bots actualizan estrategias tras cambiar rama.
[OK] Los bots reciben equipo de la rama correspondiente.
[OK] Los bots nuevos aparecen con equipo adecuado.
[OK] Las gemas aparecen en items con sockets.
[OK] Los encantamientos aparecen en items equipados.
[OK] El Paladin tanque actua como tanque.
[OK] El Chaman Elemental se mantiene a distancia.
[OK] Los casters/ranged ya no se meten tanto a melee.
[OK] Follow masivo funciona.
[OK] Stay masivo funciona.
[OK] La cola de comandos evita spam.
[OK] El servidor compila correctamente tras los arreglos.

================================================================================
18. CONFIGURACION IMPORTANTE
================================================================================

aiplayerbot.conf:
--------------------------------------------------------------------------------
Configuracion corregida:

  AiPlayerbot.minEnchantingBotLevel = 60

NO usar para TBC:

  AiPlayerbot.minEnchantingBotLevel = 81

porque desactiva los encantamientos en bots nivel 70.

Otras configuraciones relevantes:
--------------------------------------------------------------------------------
  AiPlayerbot.RandomBotMinLevel = 70
  AiPlayerbot.RandomBotMaxLevel = 70
  AiPlayerbot.RandomGearMaxLevel = 500
  AiPlayerbot.RandomGearMaxDiff = 11
  AiPlayerbot.AutoPickTalents = full

================================================================================
19. TABLAS DE BASE DE DATOS RELACIONADAS
================================================================================

Tablas usadas por el sistema:
--------------------------------------------------------------------------------
- ai_playerbot_enchants
  Encantamientos por clase, spec y slot.

- ai_playerbot_weightscales
  Pesos de stats por clase/spec.

- ai_playerbot_weightscale_data
  Detalle de stats ponderadas.

- ai_playerbot_equip_cache
  Cache de equipo calculado.

- ai_playerbot_item_info_cache
  Cache de informacion de items.

COMPROBADO:
--------------------------------------------------------------------------------
La tabla ai_playerbot_enchants tenia datos completos y se confirmo que el
problema de encantamientos era configuracion, no base de datos.

================================================================================
20. RESUMEN FINAL
================================================================================

RESULTADO FINAL:
--------------------------------------------------------------------------------
Los npcbots ahora pueden:

1. Cambiar de talentos por rama necesaria.
2. Cambiar de rol segun talentos.
3. Recargar estrategias despues del cambio.
4. Equiparse segun la rama activa.
5. Nacer con equipo adecuado para su spec.
6. Llevar gemas en items con sockets.
7. Llevar encantamientos en items equipados.
8. Mantener comportamiento correcto como tanque, healer o DPS.
9. Mantener distancia si son casters/ranged.
10. Funcionar mejor en grupos y raids.

CAMBIOS MAS IMPORTANTES:
--------------------------------------------------------------------------------
- Cambio de talentos funcional.
- Auto equipamiento por spec.
- Equipamiento inicial por spec.
- Encantamientos habilitados.
- Gemas comprobadas.
- Paladin Proteccion funcionando como tanque.
- Chaman Elemental funcionando como caster ranged.
- Casters/ranged con mejor posicionamiento.
- Control masivo de grupo/raid.

================================================================================
DOCUMENTACION DE ARREGLOS Y SCRIPTS - MAZMORRA: ZUL'FARRAK
================================================================================
Proyecto      : CMaNGOS TBC (rama oficial mangos-tbc, actualizada)
Mazmorra      : Zul'Farrak
Map ID        : 209
Estado final  : COMPILADO (Release x64) y PROBADO EN JUEGO - FUNCIONA OK
================================================================================

1. OBJETIVO
--------------------------------------------------------------------------------
Completar Zul'Farrak: scriptear los 5 jefes que faltaban en el core, respetando
los scripts e instancia ya existentes, con textos en ESPANOL y voces de troll
autenticas (Zul'Aman).

2. ESTADO PREVIO DEL CORE (lo que YA existia y se respeto)
--------------------------------------------------------------------------------
- boss_zumrah.cpp            : Zumrah (7271) con tumbas someras y zombies.
- instance_zulfarrak.cpp     : 9 encounters, evento piramide, tumbas, puerta
                               final (GO_END_DOOR).
- zulfarrak.cpp              : evento del gong (invoca Gahz'rilla por DB
                               script), evento unlocking (piramide),
                               areatrigger 1447 (aggro de Antu'sul).
- zulfarrak.h                : enums, entries y clase instance_zulfarrak.

3. ARCHIVOS CREADOS (5 scripts nuevos)
--------------------------------------------------------------------------------
Ruta: src/game/AI/ScriptDevAI/scripts/kalimdor/zulfarrak/
  - boss_antusul.cpp
  - boss_theka_the_martyr.cpp
  - boss_gahzrilla.cpp
  - boss_sezzziz.cpp
  - boss_chief_ukorz_sandscalp.cpp

4. ARCHIVOS MODIFICADOS
--------------------------------------------------------------------------------
- src/game/AI/ScriptDevAI/scripts/system/ScriptLoader.cpp
    * 5 declaraciones: extern void AddSC_boss_<nombre>();
    * 5 llamadas dentro de AddScripts() en la seccion Kalimdor.
- Re-ejecucion de CMake (cmake -S . -B build) para que el target "game"
  recogiera los .cpp nuevos.

5. JEFES, ENTRIES Y MECANICAS IMPLEMENTADAS
--------------------------------------------------------------------------------
5.1 Antu'sul (entry 8127) - señor de los sul'lithuz
    - Healing Wave (12491) al aliado con menos vida (DoSelectLowestHpFriendly).
    - Earth Shock (8045) y Thunderclap (8198).
    - Invoca 2 Servant of Antu'sul (8156) dos veces por combate
      (al 60% y al 30% de vida) con TEMPSPAWN_TIMED_OOC_DESPAWN.
5.2 Theka el Martir (entry 7272)
    - Fevered Plague (8600, VERIFICADO con .lookup spell).
    - Theka Transform (11089, VERIFICADO) al 25% de vida: inmunidad
      fisico/sombra durante 30 s (mecanica clasica).
5.3 Gahz'rilla (entry 7273) - hidra invocada por el gong
    - Icicle (11131, VERIFICADO) con ralentizacion.
    - Frost Breath (21009; ajustable a 16009 si pega fuerte o 22479 si flojo).
    - Gahz'rilla Slam (11902, VERIFICADO) que repele.
    - Sin voces: usa BOSS EMOTES (type 3); sus rugidos los pone el modelo.
5.4 Sacerdote Sombrio Sezz'ziz (entry 7275)
    - Shadow Word: Pain (10892), Mind Blast (8105).
    - Renew (6078) sobre si mismo por debajo del 60% de vida.
    - Mantiene distancia de caster (m_attackDistance = 20.0f).
5.5 Jefe Ukorz Cabellarena (entry 7267) - jefe final
    - Cleave (11609) y Whirlwind (1680).
    - Frenzy/Enrage (8599) al 30% de vida con grito propio.
    (Ruuzlu ya viene como add spawnado en base de datos junto a el.)

6. SQL APLICADO (base tbcmangos)
--------------------------------------------------------------------------------
6.1 Asignacion de ScriptName en creature_template:
    UPDATE creature_template SET ScriptName='boss_antusul'                 WHERE entry=8127;
    UPDATE creature_template SET ScriptName='boss_theka_the_martyr'        WHERE entry=7272;
    UPDATE creature_template SET ScriptName='boss_gahzrilla'               WHERE entry=7273;
    UPDATE creature_template SET ScriptName='boss_sezzziz'                 WHERE entry=7275;
    UPDATE creature_template SET ScriptName='boss_chief_ukorz_sandscalp'   WHERE entry=7267;

6.2 Textos nuevos en espanol (INSERT en script_texts, entries -1209004 a
    -1209020; convencion -1 + map 209 + correlativo):
    - Antu'sul   : -1209004 aggro / -1209005 summon / -1209006 slay / -1209007 death
    - Theka      : -1209008 aggro / -1209009 transform / -1209010 slay / -1209011 death
    - Gahz'rilla : -1209012 emote aggro / -1209013 emote death (type 3, sin voz)
    - Sezz'ziz   : -1209014 aggro / -1209015 slay / -1209016 death
    - Ukorz      : -1209017 aggro / -1209018 slay / -1209019 enrage / -1209020 death

6.3 Sonidos finales (UPDATE de columna sound por entry):
    ver seccion 7.

6.4 Nota: los textos oficiales de Zumrah (-1209000 a -1209003) se conservan
    tal cual (ingles original); solo se les asigno voz. Traducibles opcional
    mediante UPDATE de content_default si se desea.

7. VOCES ASIGNADAS (SoundEntries de trolls de Zul'Aman, verificados en la
   propia tabla script_texts del core / cliente 2.4.3)
--------------------------------------------------------------------------------
Jefe       | Voz donante | IDs de sonido
-----------+-------------+-----------------------------------------------------
Antu'sul   | Akil'zon    | 12013 aggro / 12014 summon / 12017 slay / 12019 death
Theka      | Nalorakk    | 12070 aggro / 12073 SAY_TOTROLL (transformacion)
           |             | 12075 slay / 12077 death
Sezz'ziz   | Malacrass   | 12041 aggro / 12043 slay / 12051 death
Ukorz      | Zul'jin     | 12091 aggro / 12098 slay / 12097 berserk / 12100 death
Zumrah     | Jan'alai    | 12034 intro / 12031 aggro / 12036 kill / 12033 summon
Gahz'rilla | (sin voz)   | emotes type 3; rugidos automaticos del modelo
Detalle tematico: la linea SAY_TOTROLL de Nalorakk (cambio a forma troll) se
uso para la transformacion de Theka; SAY_SUMMON de Akil'zon y
SAY_SUMMON_HATCHER de Jan'alai para las invocaciones.
Metodo: el comando .lookup sound NO existe en este core; los IDs se extrajeron
de la propia tabla script_texts (SELECT entry, sound, comment WHERE sound<>0).

8. VERIFICACION DE HECHIZOS EN JUEGO (.lookup spell)
--------------------------------------------------------------------------------
- Theka Transform   = 11089
- Icicle            = 11131
- Gahz'rilla Slam   = 11902
- Fevered Plague    = 8600   (OJO: NO es 16168; 16168 es otra variante)
- Frost Breath      = varios rangos (3129, 3131, 16004, 16009, 16340, 21009,
                      22479, 29318...); se eligio 21009 por nivel del jefe.

9. PROBLEMAS TECNICOS Y SOLUCION (lecciones aplicables a todo el proyecto)
--------------------------------------------------------------------------------
9.1 Enum de invocaciones renombrado en CMaNGOS moderno: TEMPSPAWN_*
    (no TEMPSUMMON_* de ScriptDev2). Usado TEMPSPAWN_TIMED_OOC_DESPAWN.
9.2 Patron de temporizadores: DoCastSpellIfCan(objetivo, spell) == CAST_OK.
9.3 DoScriptText(ID_TEXTO, m_creature) en ese orden (texto, criatura).
9.4 MySQL 8: columna `rank` con backticks; la columna map NO existe en
    creature_template (vivir en la tabla de spawns, creature).
9.5 .lookup sound no existe: los IDs de voz se obtienen de la propia tabla
    script_texts o verificando en Wowhead TBC.

10. PRUEBAS REALIZADAS
--------------------------------------------------------------------------------
- Compilacion Release x64 sin errores.
- Prueba en juego de las 4 alas y de los eventos del gong y la piramide.
  RESULTADO: OK (confirmado por el desarrollador: "funcionaron muy bien").

================================================================================
DOCUMENTACION DE ARREGLOS Y SCRIPTS - MAZMORRA: LA CARCEL DE VENTORRENTA
================================================================================
Proyecto      : CMaNGOS TBC (rama oficial mangos-tbc, actualizada)
Mazmorra      : The Stockade / La Carcel de Ventormenta
Map ID        : 34
Estado final  : COMPILADO (Release x64) y PROBADO EN JUEGO - FUNCIONA OK
================================================================================

1. OBJETIVO
--------------------------------------------------------------------------------
Scriptear los 6 jefes de La Carcel (que carecian de script en el core), con:
- Mecanicas de combate propias de cada jefe.
- Textos (gritos) en ESPANOL.
- Voces (SoundEntries) verificadas contra el cliente 2.4.3.

2. ARCHIVOS CREADOS (6 scripts nuevos)
--------------------------------------------------------------------------------
Ruta: src/game/AI/ScriptDevAI/scripts/eastern_kingdoms/stockade/
  - boss_targorr_the_dread.cpp
  - boss_kam_deepfury.cpp
  - boss_hamhock.cpp
  - boss_dextren_ward.cpp
  - boss_bazil_thredd.cpp
  - boss_bruegal_ironknuckle.cpp

3. ARCHIVOS MODIFICADOS
--------------------------------------------------------------------------------
- src/game/AI/ScriptDevAI/scripts/system/ScriptLoader.cpp
    * 6 declaraciones: extern void AddSC_boss_<nombre>();
    * 6 llamadas dentro de AddScripts() en la seccion Eastern Kingdoms.
- Re-ejecucion de CMake (cmake -S . -B build) para que el target "game"
  recogiera los .cpp nuevos (el glob no se refresca solo).

4. JEFES, ENTRIES Y MECANICAS IMPLEMENTADAS
--------------------------------------------------------------------------------
4.1 Targorr el Horror (entry 1696) - caster oscuro
    - Shadow Bolt (spell 1088) a objetivo.
    - Fear / Miedo (spell 5782) cada 12-16 s.
4.2 Kam Profundauria (entry 1666) - guerrero humano
    - Mortal Strike (12294), Whirlwind (1680), Intimidating Shout (5246).
4.3 Hamhock (entry 1717) - ogro brutal
    - Cleave (845), Stomp (5589).
    - Enrage (8599) al 30% de vida, con grito propio.
4.4 Dextren Ward (entry 1663) - lider prisionero
    - Rend (11572), Net / Red (6534).
    - Al 50% de vida invoca 2 Defias Inmate (entry 1727) con
      TEMPSPAWN_TIMED_OOC_DESPAWN (despawn al salir de combate).
4.5 Bazil Thredd (entry 1716) - carcelero
    - Hamstring (1715).
    - Charge / Carga (100) solo si el objetivo esta a mas de 8.0f
      (comprobado con IsWithinDistInMap).
    - Execute (5308) por debajo del 20% de vida.
4.6 Bruegal Puno de Hierro (entry 1720) - ogro tanque final
    - Uppercut (10966), Thunderclap (6343), Knock Away (10101).
    - Enrage (8599) al 25% de vida, con grito propio.

5. SQL APLICADO (base tbcmangos)
--------------------------------------------------------------------------------
5.1 Asignacion de ScriptName en creature_template:
    UPDATE creature_template SET ScriptName='boss_targorr_the_dread'    WHERE entry=1696;
    UPDATE creature_template SET ScriptName='boss_kam_deepfury'         WHERE entry=1666;
    UPDATE creature_template SET ScriptName='boss_hamhock'              WHERE entry=1717;
    UPDATE creature_template SET ScriptName='boss_dextren_ward'         WHERE entry=1663;
    UPDATE creature_template SET ScriptName='boss_bazil_thredd'         WHERE entry=1716;
    UPDATE creature_template SET ScriptName='boss_bruegal_ironknuckle'  WHERE entry=1720;
5.2 Textos en espanol: INSERT en script_texts, entries -1034000 a -1034020
    (convencion -1 + map 034 + correlativo). type=1 (grito) / 0 (decir).
5.3 Sonidos: UPDATE de columna sound por entry (ver seccion 6).

6. VOCES ASIGNADAS (SoundEntries verificados en cliente 2.4.3)
--------------------------------------------------------------------------------
Jefe      | Voz donante            | IDs de sonido
----------+------------------------+------------------------------------------
Targorr   | Shade of Aran (humano) | 9324 aggro / 9243 slay / 9252 death
Kam       | Moroes (no-muerto)     | 9211 aggro / 9214 slay / 9216 death
Hamhock   | High King Maulgar      | 11367 aggro / 11373 slay / 11369 death
          | (ogro)                 | 11368 enrage
Dextren   | Exarch Maladaar        | 10515 aggro y slay / 10510 summon y death
Bazil     | Kargath Bladefist      | 10325 aggro / 10327 slay / 10328 death
Bruegal   | High King Maulgar      | 11367 aggro / 11374 slay / 11370 death
          | (ogro, variantes)      | 11368 enrage
Metodo de verificacion: consulta a la propia tabla script_texts del core
(IDs sniffeados por CMaNGOS) + contraste con Wowhead TBC.

7. PRUEBAS REALIZADAS
--------------------------------------------------------------------------------
- Prueba en juego de los 6 jefes: gritos en espanol con voz, habilidades
  y temporizadores correctos. RESULTADO: OK (confirmado por el desarrollador).


================================================================================
                    DOCUMENTACIÓN DE ADDON - BAGNON
                    WoW TBC 2.4.3 - Versión en Español
================================================================================

DESCRIPCIÓN GENERAL
-------------------
Bagnon es un addon que reemplaza las bolsas predeterminadas del juego por una 
interfaz unificada y personalizable que muestra todo tu inventario en una 
sola ventana.


MODIFICACIONES REALIZADAS
--------------------------

SISTEMA DE FILTRADO POR CALIDAD DE ITEMS
Se implementó un sistema de botones de filtro que permite mostrar/ocultar 
items según su calidad (rareza):

  • Gris (Poor)         - Items basura
  • Blanco (Common)     - Items comunes
  • Verde (Uncommon)    - Items poco comunes
  • Azul (Rare)         - Items raros
  • Morado (Epic)       - Items épicos
  • Naranja (Legendary) - Items legendarios

CARACTERÍSTICAS DE LOS FILTROS:
  • Botones ubicados en la esquina superior derecha de la ventana
  • Cada botón muestra el color correspondiente a la calidad
  • Estado activo: Opacidad 100% (brillo completo)
  • Estado inactivo: Opacidad 30% (semi-transparente)
  • Los filtros se aplican en tiempo real al inventario


COMANDOS PRINCIPALES
--------------------

/bagnon           - Abre/cierra la ventana principal de Bagnon
/bagnon config    - Abre el panel de configuración


GUÍA DE USO
-----------

1. ABRIR BAGNON:
   • Usa /bagnon
   • O haz clic en el ícono de la bolsa en la barra de acciones

2. ACTIVAR/DESACTIVAR FILTROS:
   • Haz clic en los botones de color en la esquina superior derecha
   • Los botones activos brillan con opacidad completa
   • Los items se filtran inmediatamente

3. CONFIGURACIÓN:
   • Click derecho en el título para opciones avanzadas
   • Doble click en el título para buscar items
   • Arrastrar el título para mover la ventana


CARACTERÍSTICAS ADICIONALES
---------------------------

✓ Redimensionable: Ajusta el número de columnas en configuración
✓ Múltiples personajes: Soporte para ver inventario de otros personajes
✓ Banco: Visualización separada del banco
✓ Búsqueda: Doble click en el título para buscar items específicos
✓ Personalizable: Colores, transparencia y posición guardados


RESUMEN DE FUNCIONALIDADES
---------------------------

FUNCIÓN                    | ACCIÓN
---------------------------|------------------------------------------------
/bagnon                    | Abrir/cerrar inventario
/bagnon config             | Abrir configuración
Click en botón de color    | Activar/desactivar filtro de calidad
Click derecho en título    | Menú de opciones
Doble click en título      | Buscar items
Arrastrar título           | Mover ventana


NOTAS TÉCNICAS
--------------

• Compatible con WoW TBC 2.4.3
• Funciona en servidores CMaNGOS y similares
• Los filtros se aplican automáticamente al activarlos
• La configuración se guarda entre sesiones


================================================================================
                    DOCUMENTACIÓN DE ADDON - PFQUEST
                    WoW TBC 2.4.3 - Mapas de Mazmorras
================================================================================

DESCRIPCIÓN GENERAL
-------------------
pfQuest es un addon completo que incluye:
  • Asistente de misiones con tracking en tiempo real
  • Base de datos de quests, items y NPCs
  • Sistema de navegación con flecha direccional
  • Mapas detallados de mazmorras (modificación personalizada)


MODIFICACIÓN REALIZADA: MAPAS DE MAZMORRAS
-------------------------------------------

Se creó un sistema que muestra mapas detallados de mazmorras de Classic y 
Burning Crusade directamente en pfQuest.

CARACTERÍSTICAS:
  • Frame independiente y flotante
  • Redimensionable (128x96 a 512x384 píxeles)
  • Movible (arrastrable a cualquier posición)
  • Guarda automáticamente posición y tamaño
  • Se abre automáticamente al entrar a una mazmorra
  • Compatible con todas las mazmorras de Classic y TBC


COMANDO PRINCIPAL
-----------------

/dmap    - Abre/cierra el mapa de la mazmorra actual


GUÍA DE USO
-----------

MÉTODO AUTOMÁTICO:
  1. Entra a cualquier mazmorra (party o raid)
  2. El mapa aparecerá automáticamente en la pantalla
  3. El mapa se posiciona en la ubicación guardada

MÉTODO MANUAL:
  1. Escribe /dmap en el chat
  2. El mapa de la mazmorra actual se abrirá
  3. Escribe /dmap nuevamente para cerrarlo

PERSONALIZACIÓN:

  MOVER EL MAPA:
    • Haz clic y arrastra desde cualquier parte del frame
    • Excepto la esquina inferior derecha (handle de redimensión)
    • Suelta en la posición deseada
    • La posición se guarda automáticamente

  REDIMENSIONAR EL MAPA:
    • Busca el ícono de redimensión en la esquina inferior derecha
    • Haz clic y arrastra para ajustar el tamaño
    • Tamaño mínimo: 128x96 píxeles
    • Tamaño máximo: 512x384 píxeles
    • El tamaño se guarda automáticamente

  CERRAR EL MAPA:
    • Click en la "X" en la esquina superior derecha
    • O escribe /dmap


MAZMORRAS SOPORTADAS
--------------------

CLASSIC (VANILLA):
  • Las Mazmorras (The Stockade)
  • Las Minas de la Muerte (The Deadmines)
  • Monasterio Escarlata (Scarlet Monastery)
  • Scholomance
  • Stratholme
  • Profundidades de Roca Negra (Blackrock Depths)
  • Cumbre de Roca Negra (Blackrock Spire)
  • Núcleo de Magma (Molten Core)
  • Guarida de Alanegra (Blackwing Lair)
  • Naxxramas
  • Profundidades de Brazanegra (Blackfathom Deeps)
  • Zahúrda Rajacieno (Razorfen Kraul)
  • Zahúrda de los Bajos (Razorfen Downs)
  • Gnomeregan
  • Maraudon
  • Uldaman
  • Zul'Farrak
  • Zul'Gurub
  • El Templo Sumergido (The Sunken Temple)
  • Cavernas de los Lamentos (Wailing Caverns)
  • Sima Ígnea (Ragefire Chasm)
  • Guarida de Onyxia (Onyxia's Lair)
  • La Masacre (Dire Maul)
  • Castillo de Colmillo Oscuro (Shadowfang Keep)
  • Ruinas de Ahn'Qiraj
  • Templo de Ahn'Qiraj

BURNING CRUSADE:
  • Las Penas del Infierno (The Slave Pens)
  • La Sotén (The Underbog)
  • La Manaforgia (The Steamvault)
  • Criptas Auchenai (Auchenai Crypts)
  • Tumbas de Maná (Mana Tombs)
  • Recinto de los Sethekk (Sethekk Halls)
  • Laberinto de las Sombras (Shadow Labyrinth)
  • Murallas de Fuego Infernal (Hellfire Ramparts)
  • El Foso de Sangre (The Blood Furnace)
  • Las Salas Arrasadas (The Shattered Halls)
  • Guarida de Magtheridon (Magtheridon's Lair)
  • Karazhan
  • Bancal del Magister (Magister's Terrace)
  • Zul'Aman
  • El Ojo (The Eye)
  • Castillo de la Tempestad (Tempest Keep)
  • Caverna de Santuario Serpiente (Serpentshrine Cavern)
  • Guarida de Gruul (Gruul's Lair)
  • Templo Oscuro (Black Temple)
  • Meseta de la Fuente del Sol (Sunwell Plateau)
  • Cavernas del Tiempo (Caverns of Time)


ESTRUCTURA DE ARCHIVOS
----------------------

pfQuest-tbc/
├── dungeonmaps.lua          ← Módulo de mapas de mazmorras
└── img/
    └── dungeons/            ← Texturas de mapas (.blp)
        ├── CL_TheStockade.blp
        ├── CL_TheDeadmines.blp
        ├── CFRTheSlavePens.blp
        └── ... (140+ mapas)


NOTAS TÉCNICAS IMPORTANTES
---------------------------

LIMITACIONES CONOCIDAS:

  1. POSICIÓN DEL JUGADOR:
     • En TBC 2.4.3, la API GetPlayerMapPosition() devuelve (0,0) dentro 
       de instancias
     • Esto es una limitación del cliente de WoW TBC, NO del addon
     • El mapa se muestra correctamente pero SIN indicador de posición 
       en tiempo real

  2. COMPATIBILIDAD:
     • Diseñado específicamente para WoW TBC 2.4.3
     • Funciona en servidores CMaNGOS y similares
     • Requiere todas las texturas .blp en la carpeta img/dungeons/


REQUISITOS
----------

  • WoW Burning Crusade 2.4.3
  • pfQuest instalado y configurado
  • Texturas de Atlas (Classic + TBC) en pfQuest-tbc/img/dungeons/


SOLUCIÓN DE PROBLEMAS
---------------------

PROBLEMA: El mapa no aparece al entrar a una mazmorra
SOLUCIÓN:
  1. Escribe /dmap manualmente
  2. Verifica que estés en una instancia (party/raid)
  3. Revisa el chat en busca de errores

PROBLEMA: "Textura no encontrada"
SOLUCIÓN:
  1. Verifica que el archivo .blp exista en pfQuest-tbc/img/dungeons/
  2. El nombre debe coincidir exactamente (ej: CL_TheStockade.blp)

PROBLEMA: El mapa no se mueve o redimensiona
SOLUCIÓN:
  • Para mover: Arrastra desde cualquier parte del frame
  • Para redimensionar: Usa el handle en la esquina inferior derecha


RESUMEN DE COMANDOS
-------------------

COMANDO    | FUNCIÓN
-----------|---------------------------------------------------------
/dmap      | Abrir/cerrar mapa de mazmorra


                    Versión: WoW TBC 2.4.3
                    Idioma: Español
                    Basado en: pfQuest (Shagu) + Atlas Maps
================================================================================
