#include "LightUniforms.hpp"

DirectionalLightUniformData::DirectionalLightUniformData(DirectionalLight light)
	: direction{light.direction}
	, ambient{light.ambient}
	, diffuse{light.diffuse}
	, specular{light.specular}
{}

DirectionalLightUniformData& DirectionalLightUniformData::operator=(DirectionalLight light)
{
	direction = light.direction;
	ambient = light.ambient;
	diffuse = light.diffuse;
	specular = light.specular;
	return *this;
}

DirectionalShadowCasterUniformData::DirectionalShadowCasterUniformData(
    DirectionalShadowCaster caster)
	: direction{caster.light().direction}
	, ambient{caster.light().ambient}
	, diffuse{caster.light().diffuse}
	, specular{caster.light().specular}
	, model_matrix{caster.model()}
	, exists{true}
{}

PointLightUniformData::PointLightUniformData(PointLight light)
	: position{light.position}
	, ambient{light.ambient}
	, diffuse{light.diffuse}
	, specular{light.specular}
	, attenuation_constant{light.attenuation.constant}
	, attenuation_linear{light.attenuation.linear}
	, attenuation_quadratic{light.attenuation.quadratic}
{}

PointLightUniformData& PointLightUniformData::operator=(PointLight light)
{
	position = light.position;
	ambient = light.ambient;
	diffuse = light.diffuse;
	specular = light.specular;
	attenuation_constant = light.attenuation.constant;
	attenuation_linear = light.attenuation.linear;
	attenuation_quadratic = light.attenuation.quadratic;
	return *this;
}


SpotLightUniformData::SpotLightUniformData(SpotLight light)
	: position{light.position}
	, direction{light.direction}
	, ambient{light.ambient}
	, diffuse{light.diffuse}
	, specular{light.specular}
	, attenuation_constant{light.attenuation.constant}
	, attenuation_linear{light.attenuation.linear}
	, attenuation_quadratic{light.attenuation.quadratic}
	, cutoff_inner{light.cutoff.inner}
	, cutoff_outer{light.cutoff.outer}
{}

SpotLightUniformData& SpotLightUniformData::operator=(SpotLight light)
{
	position = light.position;
	direction = light.direction;
	ambient = light.ambient;
	diffuse = light.diffuse;
	specular = light.specular;
	attenuation_constant = light.attenuation.constant;
	attenuation_linear = light.attenuation.linear;
	attenuation_quadratic = light.attenuation.quadratic;
	cutoff_inner = light.cutoff.inner;
	cutoff_outer = light.cutoff.outer;
	return *this;
}
