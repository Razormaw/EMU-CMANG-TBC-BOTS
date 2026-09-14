/* CMaNGOS TBC - The Stockade (map 34): Bazil Thredd */

#include "AI/ScriptDevAI/include/sc_common.h"

enum
{
    SPELL_HAMSTRING               = 1715,
    SPELL_CHARGE                  = 100,
    SPELL_EXECUTE                 = 5308,

    SAY_AGGRO                     = -1034014,
    SAY_SLAY                      = -1034015,
    SAY_DEATH                     = -1034016,
};

struct boss_bazil_threddAI : public ScriptedAI
{
    boss_bazil_threddAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiHamstringTimer;
    uint32 m_uiChargeTimer;
    uint32 m_uiExecuteTimer;

    void Reset() override
    {
        m_uiHamstringTimer = urand(7000, 11000);
        m_uiChargeTimer = urand(16000, 22000);
        m_uiExecuteTimer = 6000;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

        void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiHamstringTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HAMSTRING) == CAST_OK)
                m_uiHamstringTimer = urand(7000, 11000);
        }
        else
            m_uiHamstringTimer -= uiDiff;

        if (m_uiChargeTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                // La Carga (Charge) requiere estar a más de 8.0f de distancia
                if (!m_creature->IsWithinDistInMap(pTarget, 8.0f))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_CHARGE) == CAST_OK)
                        m_uiChargeTimer = urand(16000, 22000);
                }
                else
                    m_uiChargeTimer = 5000; // reintenta más tarde si está muy cerca
            }
        }
        else
            m_uiChargeTimer -= uiDiff;

        if (m_creature->GetHealthPercent() < 20.0f)
        {
            if (m_uiExecuteTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_EXECUTE) == CAST_OK)
                    m_uiExecuteTimer = urand(6000, 10000);
            }
            else
                m_uiExecuteTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_bazil_thredd(Creature* pCreature)
{
    return new boss_bazil_threddAI(pCreature);
}

void AddSC_boss_bazil_thredd()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_bazil_thredd";
    pNewScript->GetAI = &GetAI_boss_bazil_thredd;
    pNewScript->RegisterSelf();
}
