
#include "Render.h"

#include "Material.h"

REArray<Material*> Material::gMaterialContainer;

void Material::Reload()
{
	for (int i = 0, ni = (int)parameterList.size(); i < ni; ++i)
	{
		parameterList[i].location = -1;
	}
}

void Material::Use(RenderContext& renderContext)
{
	if (!shader)
		return;

	// use shader
	if (!renderContext.currentMaterial || renderContext.currentMaterial->shader != shader)
		shader->Use();

	bool bNewMat = (renderContext.currentMaterial != this);

	// set parameters
	char* paramDataPtr = parameterData.data();
	for (int i = 0, ni = (int)parameterList.size(); i < ni; ++i)
	{
		MaterialParameter& param = parameterList[i];
		if (bNewMat || param.bDirty)
		{
			param.bDirty = false;
			param.SendValue(shader, paramDataPtr);
		}
	}

	// set render context
	renderContext.currentMaterial = this;
}

void Material::SetParameter(const char* name, char* data, int bytes, EMaterialParameterType type)
{
	MaterialParameter* params = 0;
	int paramListSize = (int)parameterList.size();
	for (int i = 0; i < paramListSize; ++i)
	{
		//if (parameterList.data()[i].name[0] == name[0])
		params = &parameterList[i];
		if (strcmp(params->name, name) == 0)
		{
			break;
		}
		else
		{
			params = 0;
		}
	}
	if (!params)
	{
		parameterList.push_back(MaterialParameter());
		params = &parameterList[paramListSize];
		strcpy_s(params->name, name);
		params->offset = (int)parameterData.size();
		params->count = bytes;
		params->type = type;
		params->bDirty = true;

		// add data
		parameterData.resize(params->offset + bytes);
		memcpy_s(parameterData.data() + params->offset, params->count, data, bytes);
	}
	else
	{
		assert(params->count == bytes);
		assert(params->type == type);
		params->bDirty = true;
		memcpy_s(parameterData.data() + params->offset, params->count, data, bytes);
	}
}