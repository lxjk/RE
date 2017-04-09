#pragma once

class Component
{
	virtual void Update(float deltaTime) {};
	virtual void UpdateEndOfFrame(float deltaTime) {};
};