/* -------------------------------------------------------------------------
 * TalentTemplatesTbc.h — talentos intercambiables (parche IA-raid v2)
 * Destino: src/game/PlayerBot/Base/TalentTemplatesTbc.h
 *
 * Sustituye a la clase respec de ai-playerbots (que NO existe en tu core)
 * usando la API estable de mangos-tbc:
 *   Player::resetTalents(bool)  /  Player::AddTalent(uint32, bool)
 *   Player::SetFreeTalentPoints(uint32)
 * ------------------------------------------------------------------------- */
#ifndef _TALENT_TEMPLATES_TBC_H
#define _TALENT_TEMPLATES_TBC_H

#include "Common.h"
#include <string>

class Player;

namespace raidai
{
    /* Una plantilla: clave textual + talentos clave de la build.
     * 'talents' lleva los hechizos de talento mas representativos;
     * una plantilla de produccion deberia listar TODOS los puntos. */
    struct TalentSpecDef
    {
        uint8       classId;     // CLASS_* de SharedDefines.h
        const char* key;         // para !bot <nombre> talents <key>
        const char* label;       // nombre para mostrar
        uint32      talents[8];  // hechizos de talento clave
        uint8       count;
    };

    // Busca plantilla por clave textual, valida contra la clase del bot.
    const TalentSpecDef* FindSpec(Player* bot, const std::string& key);

    // Re-especifica al bot fuera de combate. Devuelve false si no existe
    // la spec, si el bot esta en combate o si el core rechaza un talento.
    bool ApplyTalentTemplate(Player* bot, const std::string& key);

    // Planes de composicion para !raid comp <plan>:
    //   estandar -> sin cambios
    //   gemelos  -> primer druida del grupo a Feral (off-tank oso)
    //   duro     -> sacerdote a Disciplina y chaman a Restauracion
    // 'master' es cualquier miembro del grupo (se usa su Group).
    std::string ApplyCompPlan(Player* master, const std::string& plan);
}

#endif
