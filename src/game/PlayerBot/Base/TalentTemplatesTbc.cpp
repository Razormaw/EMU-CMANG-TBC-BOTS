/* -------------------------------------------------------------------------
 * TalentTemplatesTbc.cpp — plantillas de talentos TBC 2.4.3
 *
 * IMPORTANTE: los IDs son los hechizos de talento caracteristicos de cada
 * spec en TBC. Verificalos contra tu extract de DBC / spell_template;
 * una plantilla de produccion debe enumerar TODOS los puntos de la build.
 * ------------------------------------------------------------------------- */
#include "TalentTemplatesTbc.h"
#include "../../Entities/Player.h"
#include "../../Entities/Group.h"
#include "../../Globals/SharedDefines.h"

namespace raidai
{
    /* ------------------------- tabla de specs ------------------------- */

    static const TalentSpecDef sSpecs[] =
    {
        /* Guerrero */
        { CLASS_WARRIOR, "proteccion", "Protección", { 12975, 23922, 20243, 12809 }, 4 }, // Ult. aliento, Embate escudo, Destrozar
        { CLASS_WARRIOR, "furia",      "Furia",      { 23881, 29801, 12328, 12292 }, 4 }, // Sed de sangre, Desenfreno
        { CLASS_WARRIOR, "armas",      "Armas",      { 12294, 20252, 12664, 12296 }, 4 }, // Golpe mortal, Interceptacion
        /* Paladín */
        { CLASS_PALADIN, "sagrado",    "Sagrado",    { 20473, 20216, 31834, 20234 }, 4 }, // Choque Sagrado, Favor divino
        { CLASS_PALADIN, "proteccion-paladin", "Protección", { 31935, 20925, 20230, 20401 }, 4 }, // Escudo del vengador
        { CLASS_PALADIN, "reprension", "Reprensión", { 35395, 20066, 20218, 20375 }, 4 }, // Golpe de cruzado, Arrepentimiento
        /* Sacerdote */
        { CLASS_PRIEST,  "disciplina", "Disciplina", { 33206, 10060, 14751, 27900 }, 4 }, // Supresión de dolor, Infusión de poder
        { CLASS_PRIEST,  "sagrado",    "Sagrado",    { 34861, 27827, 14893, 27789 }, 4 }, // Círculo de sanación
        { CLASS_PRIEST,  "sombra",     "Sombra",     { 15473, 34914, 15487, 15407 }, 4 }, // Forma de las Sombras, Toque vampírico
        /* Mago */
        { CLASS_MAGE,    "arcano",     "Arcano",     { 12043, 12042, 31579, 12051 }, 4 }, // Presteza mental, Poder arcano
        { CLASS_MAGE,    "fuego",      "Fuego",      { 28682, 31661, 11129, 31687 }, 4 }, // Combustión, Aliento de dragón
        { CLASS_MAGE,    "escarcha",   "Escarcha",   { 31687, 12472, 11958, 12952 }, 4 }, // Elemental de agua, Venas heladas
        /* Pícaro */
        { CLASS_ROGUE,   "asesinato",  "Asesinato",  { 1329, 14177, 13705, 14117 }, 4 },  // Mutilar, Sangre fría
        { CLASS_ROGUE,   "combate",    "Combate",    { 13877, 13750, 35551, 13852 }, 4 }, // Ráfaga de acero, Subidón de adrenalina
        { CLASS_ROGUE,   "sutileza",   "Sutileza",   { 14183, 14185, 14278, 13983 }, 4 }, // Premeditación, Preparación
        /* Cazador */
        { CLASS_HUNTER,  "bestias",    "Bestias",    { 19574, 34692, 34953, 19506 }, 4 }, // Cólera de las bestias
        { CLASS_HUNTER,  "punteria",   "Puntería",   { 19434, 3045, 19506, 20905 }, 4 },  // Disparo de puntería, Fuego rápido
        { CLASS_HUNTER,  "supervivencia", "Supervivencia", { 19386, 34490, 34500, 19373 }, 4 }, // Picadura de dracoleón
        /* Brujo */
        { CLASS_WARLOCK, "afliccion",  "Aflicción",  { 30108, 18223, 18288, 17877 }, 4 }, // Aflicción inestable
        { CLASS_WARLOCK, "demonologia","Demonología",{ 19028, 18708, 18094, 17877 }, 4 }, // Enlace de alma
        { CLASS_WARLOCK, "destruccion","Destrucción",{ 17962, 30283, 17877, 18265 }, 4 }, // Conflagración, Quemadura
        /* Chamán */
        { CLASS_SHAMAN,  "elemental",  "Elemental",  { 30706, 16166, 30675, 29062 }, 4 }, // Tótem de cólera
        { CLASS_SHAMAN,  "mejora",     "Mejora",     { 17364, 30823, 30798, 16281 }, 4 }, // Golpe de tormenta
        { CLASS_SHAMAN,  "restauracion", "Restauración", { 974, 16190, 30005, 16213 }, 4 }, // Escudo de tierra
        /* Druida */
        { CLASS_DRUID,   "equilibrio", "Equilibrio", { 24858, 33831, 33786, 33603 }, 4 }, // Forma de lechúcico
        { CLASS_DRUID,   "feral",      "Feral",      { 33745, 16857, 33851, 22812 }, 4 }, // Lacerar, Piel gruesa (oso)
        { CLASS_DRUID,   "restauracion-druida", "Restauración", { 33891, 18562, 33763, 34123 }, 4 }, // Árbol de vida
    };

    static const size_t sSpecCount = sizeof(sSpecs) / sizeof(sSpecs[0]);

    /* ----------------------------- API -------------------------------- */

    const TalentSpecDef* FindSpec(Player* bot, const std::string& key)
    {
        if (!bot)
            return NULL;

        for (size_t i = 0; i < sSpecCount; ++i)
        {
            if (sSpecs[i].classId != bot->getClass())
                continue;
            if (stricmp(sSpecs[i].key, key.c_str()) == 0)
                return &sSpecs[i];
        }
        return NULL;
    }

    bool ApplyTalentTemplate(Player* bot, const std::string& key)
    {
        if (!bot || bot->IsInCombat())
            return false;

        const TalentSpecDef* spec = FindSpec(bot, key);
        if (!spec)
            return false;

        // API estable del core mangos-tbc (la via que NO depende de
        // la clase respec de ai-playerbots):
        bot->resetTalents(true);           // reembolso sin coste
        bot->SetFreeTalentPoints(0);       // partida limpia

        for (uint8 i = 0; i < spec->count; ++i)
        {
            // AddTalent(id, learning) aprende el hechizo del talento.
            // Si tu fork expone otra firma, este es el UNICO punto a tocar.
            bot->AddTalent(spec->talents[i], true);
        }

        return true;
    }

    /* ------------------- planes de composicion ------------------------- */

    // Busca el primer bot del grupo con la clase pedida.
    static Player* FirstBotOfClass(Player* master, uint8 classId)
    {
        if (!master)
            return NULL;
        Group* grp = master->GetGroup();
        if (!grp)
            return NULL;
        for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
        {
            Player* m = ref->getSource();
            if (m && m != master && m->getClass() == classId)
                return m;
        }
        return NULL;
    }

    std::string ApplyCompPlan(Player* master, const std::string& plan)
    {
        if (plan == "estandar" || plan.empty())
            return "Composición estándar: sin cambios de talentos.";

        if (plan == "gemelos")
        {
            Player* druida = FirstBotOfClass(master, CLASS_DRUID);
            if (!druida)
                return "Plan 'gemelos': no hay ningún druida en el grupo.";
            if (!ApplyTalentTemplate(druida, "feral"))
                return std::string("Plan 'gemelos': ") + druida->GetName() + " está en combate.";
            return std::string("Plan 'gemelos': ") + druida->GetName() +
                   " re-especificado a Feral (off-tank oso).";
        }

        if (plan == "duro")
        {
            std::string out = "Plan 'duro': ";
            Player* priest = FirstBotOfClass(master, CLASS_PRIEST);
            Player* shaman = FirstBotOfClass(master, CLASS_SHAMAN);
            bool any = false;
            if (priest && ApplyTalentTemplate(priest, "disciplina"))
            {
                out += priest->GetName(); out += " -> Disciplina; "; any = true;
            }
            if (shaman && ApplyTalentTemplate(shaman, "restauracion"))
            {
                out += shaman->GetName(); out += " -> Restauración."; any = true;
            }
            if (!any)
                out += "no se encontró sacerdote ni chamán disponibles.";
            return out;
        }

        return "Planes disponibles: estandar, gemelos, duro";
    }
}
