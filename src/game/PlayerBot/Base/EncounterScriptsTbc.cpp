/* -------------------------------------------------------------------------
 * EncounterScriptsTbc.cpp — guiones de encuentro TBC 2.4.3
 *
 * Los entries y spell IDs son los canonicos de TBC: verifica ambos contra
 * tu base de datos (SELECT entry FROM creature_template WHERE name LIKE ...).
 * ------------------------------------------------------------------------- */
#include "EncounterScriptsTbc.h"
#include "../../Entities/Player.h"
#include <ctime>

namespace raidai
{
    /* ------------------------- tabla de guiones ------------------------ */

    static const EncounterDef sEncounters[] =
    {
        { "gruul",       19044, { 33654, 33919, 0, 0 }, 2, 33525, 12.0f }, // Machaque; area del Golpe en el suelo
        { "magtheridon", 17257, { 30510, 0, 0, 0 },     1, 30616, 10.0f }, // Salva de Descarga; Nova explosiva
        { "hydross",     21216, { 0, 0, 0, 0 },         0, 38246,  8.0f }, // Lodo vil: zona alrededor del afectado
        { "vashj",       21212, { 40088, 0, 0, 0 },     1, 38280,  9.0f }, // Rayo bifurcado a cortar; Carga estatica
        { "illidan",     22917, { 41917, 40683, 0, 0 }, 2, 40932, 11.0f }, // Demoniaco de las Sombras; Llamas de Azzinoth
        { "archimonde",  17968, { 31970, 0, 0, 0 },     1, 31945, 10.0f }, // Fuego letal: zona que se mueve
        { "kazrogal",    17888, { 31447, 0, 0, 0 },     1, 31340, 10.0f }, // Marca; Lluvia de fuego
        { "teron",       22871, { 40239, 40243, 0, 0 }, 2, 40251,  9.0f }, // Incinerar; Sombra de la muerte
    };

    static const size_t sEncounterCount = sizeof(sEncounters) / sizeof(sEncounters[0]);
    static const EncounterDef* sActive = NULL;

    /* ------------------------- turnos de corte ------------------------- */

    static uint64 sCutter      = 0;   // guid del bot con permiso de corte
    static uint32 sCutterUntil = 0;   // expiracion del turno (segundos)

    bool InterruptTurnAllowed(Player& bot)
    {
        uint32 now = (uint32)time(NULL);
        uint64 me  = bot.GetObjectGuid().GetRawValue();

        // Otro bot tiene el turno vigente: espera tu ventana.
        if (sCutter != 0 && sCutter != me && now <= sCutterUntil)
            return false;

        // Tomo el turno durante 6 segundos.
        sCutter = me;
        sCutterUntil = now + 6;
        return true;
    }

    /* ----------------------------- API --------------------------------- */

    bool LoadEncounterScript(const std::string& key)
    {
        sActive = NULL;
        for (size_t i = 0; i < sEncounterCount; ++i)
        {
            if (stricmp(sEncounters[i].key, key.c_str()) == 0)
            {
                sActive = &sEncounters[i];
                return true;
            }
        }
        return false;
    }

    const EncounterDef* ActiveEncounter()
    {
        return sActive;
    }
}
