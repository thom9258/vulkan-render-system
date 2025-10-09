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

DirectionalShadowCasterUniformData::DirectionalShadowCasterUniformData(
    DirectionalShadowCaster caster)
	: direction{caster.light().direction}
	, ambient{caster.light().ambient}
	, diffuse{caster.light().diffuse}
	, specular{caster.light().specular}
	, viewproj_matrix{caster.projection().get() * caster.view()}
	, exists{true}
{}

SpotShadowCasterUniformData::SpotShadowCasterUniformData(
    SpotShadowCaster caster)
	: position{caster.light().position}
	, direction{caster.light().direction}
	, ambient{caster.light().ambient}
	, diffuse{caster.light().diffuse}
	, specular{caster.light().specular}
	, attenuation_constant(caster.light().attenuation.constant)
	, attenuation_linear(caster.light().attenuation.linear)
	, attenuation_quadratic(caster.light().attenuation.quadratic)
	, cutoff_inner(caster.light().cutoff.inner)
	, cutoff_outer(caster.light().cutoff.outer)
	, viewproj_matrix{caster.projection().get() * caster.view()}
	, exists{true}
{}


