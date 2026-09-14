#include "AI/ScriptDevAI/include/sc_common.h"
#include "zulfarrak.h"

enum
{
    EMOTE_AGGRO               = -1209012,
    EMOTE_DEATH               = -1209013,

    SPELL_ICICLE              = 11131,    // VERIFICADO en tu cliente
    SPELL_FROST_BREATH        = 21009,    // rango elegido del lookup (cono + aturdimiento)
    SPELL_GAHZRILLA_SLAM      = 11902,    // VERIFICADO en tu cliente
};

struct boss_gahzrillaAI : public ScriptedAI
{
    boss_gahzrillaAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiIcicleTimer;
    uint32 m_uiBreathTimer;
    uint32 m_uiSlamTimer;

    void Reset() override
    {
        m_uiIcicleTimer = urand(6000, 10000);
        m_uiBreathTimer = urand(12000, 18000);
        m_uiSlamTimer = urand(15000, 22000);
    }

    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiIcicleTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_ICICLE) == CAST_OK)
                    m_uiIcicleTimer = urand(6000, 10000);
            }
        }
        else
            m_uiIcicleTimer -= uiDiff;

        if (m_uiBreathTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FROST_BREATH) == CAST_OK)
                m_uiBreathTimer = urand(12000, 18000);
        }
        else
            m_uiBreathTimer -= uiDiff;

        if (m_uiSlamTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_GAHZRILLA_SLAM) == CAST_OK)
                m_uiSlamTimer = urand(15000, 22000);
        }
        else
            m_uiSlamTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_gahzrilla(Creature* pCreature)
{
    return new boss_gahzrillaAI(pCreature);
}

void AddSC_boss_gahzrilla()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_gahzrilla";
    pNewScript->GetAI = &GetAI_boss_gahzrilla;
    pNewScript->RegisterSelf();
}
