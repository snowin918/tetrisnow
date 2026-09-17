#include "Game/Player.h"

Player::Player(std::string name)
    : m_name(std::move(name))
{
}

bool Player::receiveAttack(const SnowAttack& attack)
{
    return m_gameManager.receiveAttack(attack);
}

void Player::reset()
{
    m_gameManager.reset();
    m_snowEnergy = 0;
}
