/*
 * digital.cpp
 *
 *  Created on: Nov 2, 2015
 *      Author: Joshua Southerland
 */

#include "wallaby/digital.hpp"
#include "digital_p.hpp"

Digital::Digital(unsigned char port)
: m_port(port)
{

}

void Digital::setValue(bool value)
{
	Private::Digital::instance()->setValue(m_port, value);
}

bool Digital::value() const
{
	return Private::Digital::instance()->value(m_port);
}

void Digital::setOutput(bool output)
{
	Private::Digital::instance()->setDirection(m_port, output ? Private::Digital::Out : Private::Digital::In);
}

bool Digital::isOutput() const
{
	return Private::Digital::instance()->direction(m_port) == Private::Digital::Out;
}

void Digital::setPullup(bool pullup)
{
	Private::Digital::instance()->setPullup(m_port, pullup);
}

bool Digital::pullup() const
{
	return Private::Digital::instance()->pullup(m_port);
}
