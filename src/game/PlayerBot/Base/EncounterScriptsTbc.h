/* -------------------------------------------------------------------------
 * EncounterScriptsTbc.h — guiones de jefe TBC como datos
 * Destino: src/game/PlayerBot/Base/EncounterScriptsTbc.h
 * ------------------------------------------------------------------------- */
#ifndef _ENCOUNTER_SCRIPTS_TBC_H
#define _ENCOUNTER_SCRIPTS_TBC_H

#include "Common.h"
#include <string>

class Player;

namespace raidai
{
    /* Un guion es pura descripción: qué jefe (entry), qué auras se
     * interrumpen, qué aura pinta una zona hostil y de qué tamaño. */
    struct EncounterDef
    {
        const char* key;
        uint32      entry;            // creature_template.entry del jefe
        uint32      interruptible[4]; // auras/casts a cortar (spell IDs)
        uint8       interruptCount;
        uint32      hazardAura;       // aura que crea zona hostil (0 = ninguna)
        float       hazardRadius;
    };

    // Carga el guion activo para el próximo pull (!raid guion <key>).
    bool LoadEncounterScript(const std::string& key);

    // Guion cargado actualmente (NULL si no hay ninguno).
    const EncounterDef* ActiveEncounter();

    // Turnos de corte: devuelve true solo al bot que tiene permiso para
    // cortar ahora (los demas guardan su cooldown).
    bool InterruptTurnAllowed(Player& bot);
}

#endif
