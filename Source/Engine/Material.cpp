
#include "Render.h"

#include "Material.h"

REArray<Material*> Material::gMaterialContainer;

void Material::Reload(Shader* inNewShader)
{
	if (inNewShader)
	{
		shader->referenceMaterials.erase(this);
		shader = inNewShader;
		shader->referenceMaterials.insert(this);
	}

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

void Material::DispatchCompute(struct RenderContext& renderContext, unsigned int x, unsigned int y, unsigned int z)
{
	if (!shader || !shader->computeFilePath[0])
		return;

	Use(renderContext);
	glDispatchCompute((GLuint)x, (GLuint)y, (GLuint)z);
}

void Material::SetParameter(const char* name, const char* data, int bytes, EMaterialParameterType type)
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

void Material::CopyParameter(const Material* otherMaterial, const REArray<char*>* names)
{
	assert(otherMaterial);
	if (names)
	{
		for (int j = 0, nj = (int)names->size(); j < nj; ++j)
		{
			const char* name = (*names)[j];
			for (int i = 0, ni = (int)otherMaterial->parameterList.size(); i < ni; ++i)
			{
				const MaterialParameter* params = &otherMaterial->parameterList[i];
				if (strcmp(params->name, name) == 0)
				{
					SetParameter(name, otherMaterial->parameterData.data() + params->offset, params->count, params->type);
					break;
				}
			}
		}
	}
	else
	{
		// copy all
		parameterList = otherMaterial->parameterList;
		int dataSize = (int)otherMaterial->parameterData.size();
		parameterData.resize(dataSize);
		memcpy_s(parameterData.data(), dataSize, otherMaterial->parameterData.data(), dataSize);
		for (int i = 0, ni = (int)parameterList.size(); i < ni; ++i)
		{
			MaterialParameter* params = &parameterList[i];
			params->location = -1;
			params->bDirty = true;
		}
	}
}