/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "register_types.h"

#include "visual_shader.h"
#include "vs_nodes/visual_shader_nodes.h"
#include "vs_nodes/visual_shader_particle_nodes.h"
#include "vs_nodes/visual_shader_sdf_nodes.h"

#include "core/object/class_db.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/visual_shader_editor_plugin.h"
#include "editor/visual_shader_language_plugin.h"

static void _editor_init() {
	Ref<EditorInspectorVisualShaderModePlugin> visual_shader_mode_plugin;
	visual_shader_mode_plugin.instantiate();
	EditorInspector::add_inspector_plugin(visual_shader_mode_plugin);

	Ref<VisualShaderConversionPlugin> visual_shader_convert;
	visual_shader_convert.instantiate();
	EditorNode::get_singleton()->add_resource_conversion_plugin(visual_shader_convert);

	Ref<VisualShaderLanguagePlugin> visual_shader_lang;
	visual_shader_lang.instantiate();
	EditorShaderLanguagePlugin::register_shader_language(visual_shader_lang);
}
#endif // TOOLS_ENABLED

void initialize_visual_shader_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		VLTR_REGISTER_CLASS(VisualShader);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNode);
		VLTR_REGISTER_CLASS(VisualShaderNodeCustom);
		VLTR_REGISTER_CLASS(VisualShaderNodeInput);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeOutput);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeResizableBase);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeGroupBase);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeConstant);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeVectorBase);
		VLTR_REGISTER_CLASS(VisualShaderNodeFrame);
#ifndef DISABLE_DEPRECATED
		VLTR_REGISTER_CLASS(VisualShaderNodeComment); // Deprecated, just for compatibility.
#endif
		VLTR_REGISTER_CLASS(VisualShaderNodeFloatConstant);
		VLTR_REGISTER_CLASS(VisualShaderNodeIntConstant);
		VLTR_REGISTER_CLASS(VisualShaderNodeUIntConstant);
		VLTR_REGISTER_CLASS(VisualShaderNodeBooleanConstant);
		VLTR_REGISTER_CLASS(VisualShaderNodeColorConstant);
		VLTR_REGISTER_CLASS(VisualShaderNodeVec2Constant);
		VLTR_REGISTER_CLASS(VisualShaderNodeVec3Constant);
		VLTR_REGISTER_CLASS(VisualShaderNodeVec4Constant);
		VLTR_REGISTER_CLASS(VisualShaderNodeTransformConstant);
		VLTR_REGISTER_CLASS(VisualShaderNodeFloatOp);
		VLTR_REGISTER_CLASS(VisualShaderNodeIntOp);
		VLTR_REGISTER_CLASS(VisualShaderNodeUIntOp);
		VLTR_REGISTER_CLASS(VisualShaderNodeVectorOp);
		VLTR_REGISTER_CLASS(VisualShaderNodeColorOp);
		VLTR_REGISTER_CLASS(VisualShaderNodeTransformOp);
		VLTR_REGISTER_CLASS(VisualShaderNodeTransformVecMult);
		VLTR_REGISTER_CLASS(VisualShaderNodeFloatFunc);
		VLTR_REGISTER_CLASS(VisualShaderNodeIntFunc);
		VLTR_REGISTER_CLASS(VisualShaderNodeUIntFunc);
		VLTR_REGISTER_CLASS(VisualShaderNodeVectorFunc);
		VLTR_REGISTER_CLASS(VisualShaderNodeColorFunc);
		VLTR_REGISTER_CLASS(VisualShaderNodeTransformFunc);
		VLTR_REGISTER_CLASS(VisualShaderNodeUVFunc);
		VLTR_REGISTER_CLASS(VisualShaderNodeUVPolarCoord);
		VLTR_REGISTER_CLASS(VisualShaderNodeDotProduct);
		VLTR_REGISTER_CLASS(VisualShaderNodeVectorLen);
		VLTR_REGISTER_CLASS(VisualShaderNodeDeterminant);
		VLTR_REGISTER_CLASS(VisualShaderNodeDerivativeFunc);
		VLTR_REGISTER_CLASS(VisualShaderNodeClamp);
		VLTR_REGISTER_CLASS(VisualShaderNodeFaceForward);
		VLTR_REGISTER_CLASS(VisualShaderNodeOuterProduct);
		VLTR_REGISTER_CLASS(VisualShaderNodeSmoothStep);
		VLTR_REGISTER_CLASS(VisualShaderNodeStep);
		VLTR_REGISTER_CLASS(VisualShaderNodeVectorDistance);
		VLTR_REGISTER_CLASS(VisualShaderNodeVectorRefract);
		VLTR_REGISTER_CLASS(VisualShaderNodeMix);
		VLTR_REGISTER_CLASS(VisualShaderNodeVectorCompose);
		VLTR_REGISTER_CLASS(VisualShaderNodeTransformCompose);
		VLTR_REGISTER_CLASS(VisualShaderNodeVectorDecompose);
		VLTR_REGISTER_CLASS(VisualShaderNodeTransformDecompose);
		VLTR_REGISTER_CLASS(VisualShaderNodeTexture);
		VLTR_REGISTER_CLASS(VisualShaderNodeCurveTexture);
		VLTR_REGISTER_CLASS(VisualShaderNodeCurveXYZTexture);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeSample3D);
		VLTR_REGISTER_CLASS(VisualShaderNodeTexture2DArray);
		VLTR_REGISTER_CLASS(VisualShaderNodeTexture3D);
		VLTR_REGISTER_CLASS(VisualShaderNodeCubemap);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeParameterRef);
		VLTR_REGISTER_CLASS(VisualShaderNodeFloatParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeIntParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeUIntParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeBooleanParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeColorParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeVec2Parameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeVec3Parameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeVec4Parameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeTransformParameter);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeTextureParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeTexture2DParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeTextureParameterTriplanar);
		VLTR_REGISTER_CLASS(VisualShaderNodeTexture2DArrayParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeTexture3DParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeCubemapParameter);
		VLTR_REGISTER_CLASS(VisualShaderNodeLinearSceneDepth);
		VLTR_REGISTER_CLASS(VisualShaderNodeWorldPositionFromDepth);
		VLTR_REGISTER_CLASS(VisualShaderNodeScreenNormalWorldSpace);
		VLTR_REGISTER_CLASS(VisualShaderNodeIf);
		VLTR_REGISTER_CLASS(VisualShaderNodeSwitch);
		VLTR_REGISTER_CLASS(VisualShaderNodeFresnel);
		VLTR_REGISTER_CLASS(VisualShaderNodeExpression);
		VLTR_REGISTER_CLASS(VisualShaderNodeGlobalExpression);
		VLTR_REGISTER_CLASS(VisualShaderNodeIs);
		VLTR_REGISTER_CLASS(VisualShaderNodeCompare);
		VLTR_REGISTER_CLASS(VisualShaderNodeMultiplyAdd);
		VLTR_REGISTER_CLASS(VisualShaderNodeBillboard);
		VLTR_REGISTER_CLASS(VisualShaderNodeDistanceFade);
		VLTR_REGISTER_CLASS(VisualShaderNodeProximityFade);
		VLTR_REGISTER_CLASS(VisualShaderNodeRandomRange);
		VLTR_REGISTER_CLASS(VisualShaderNodeRemap);
		VLTR_REGISTER_CLASS(VisualShaderNodeRotationByAxis);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeVarying);
		VLTR_REGISTER_CLASS(VisualShaderNodeVaryingSetter);
		VLTR_REGISTER_CLASS(VisualShaderNodeVaryingGetter);
		VLTR_REGISTER_CLASS(VisualShaderNodeReroute);

		VLTR_REGISTER_CLASS(VisualShaderNodeSDFToScreenUV);
		VLTR_REGISTER_CLASS(VisualShaderNodeScreenUVToSDF);
		VLTR_REGISTER_CLASS(VisualShaderNodeTextureSDF);
		VLTR_REGISTER_CLASS(VisualShaderNodeTextureSDFNormal);
		VLTR_REGISTER_CLASS(VisualShaderNodeSDFRaymarch);

		VLTR_REGISTER_CLASS(VisualShaderNodeParticleOutput);
		VLTR_REGISTER_ABSTRACT_CLASS(VisualShaderNodeParticleEmitter);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleSphereEmitter);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleBoxEmitter);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleRingEmitter);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleMeshEmitter);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleMultiplyByAxisAngle);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleConeVelocity);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleRandomness);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleAccelerator);
		VLTR_REGISTER_CLASS(VisualShaderNodeParticleEmit);
#ifdef TOOLS_ENABLED
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorNode::add_init_callback(_editor_init);
#endif // TOOLS_ENABLED
	}
}

void uninitialize_visual_shader_module(ModuleInitializationLevel p_level) {}
