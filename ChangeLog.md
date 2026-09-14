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

7. PROBLEMAS TECNICOS ENCONTRADOS Y SOLUCION (LECCIONES)
--------------------------------------------------------------------------------
7.1 error C2039: "IsWithinMeleeRange" no es miembro de Creature.
    SOLUCION: usar m_creature->IsWithinDistInMap(pTarget, 8.0f).
7.2 error C2065: TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT / _TIMED_OR_DEAD_ /
    _TIMED_DESPAWN no declarados.
    CAUSA: en CMaNGOS moderno el enum paso a TempSpawnType con prefijo
    TEMPSPAWN_ (antes TEMPSUMMON_ de ScriptDev2).
    SOLUCION: usar TEMPSPAWN_TIMED_OOC_DESPAWN (verificado en
    CreatureEventAI.cpp y SpellEffects.cpp del propio repo).
7.3 MySQL 8.0.41: error 1064 por la columna rank (palabra reservada).
    SOLUCION: escaparla con backticks: `rank`.
7.4 error 1054: columna 'map' desconocida en creature_template.
    CAUSA: el map vive en la tabla de spawns (creature), no en template.
    SOLUCION: JOIN creature_template con creature, o busqueda por name.
7.5 El comando .lookup sound NO existe en este core.
    SOLUCION: extraer IDs de sonido de la propia tabla script_texts
    (WHERE sound <> 0) o verificar en Wowhead TBC.
7.6 Estilo de API obligatorio en esta rama:
    - #include "AI/ScriptDevAI/include/sc_common.h"
    - DoCastSpellIfCan(objetivo, spell) == CAST_OK para temporizadores.
    - DoScriptText(ID_TEXTO, m_creature)  (orden: texto, criatura).
    - GetAI devuelve UnitAI*; registro con Script* pNewScript = new Script;
      pNewScript->Name / ->GetAI / ->RegisterSelf().

8. PRUEBAS REALIZADAS
--------------------------------------------------------------------------------
- Compilacion Release x64 sin errores tras correcciones 7.1 y 7.2.
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
