/**************************************************************************/
/*  xr_face_modifier_3d.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "servers/xr/xr_tracker.h"
#include "servers/xr/xr_server.h"
#include "xr_face_modifier_3d.h"

// This method takes the name of a mesh blend shape and returns the
// corresponding XRFaceTracker blend shape. If no match is
// found then the function returns -1.
static int find_face_blend_shape(const String& p_name)
{
	// Entry for blend shape name table.
	struct blend_map_entry
	{
		int blend;
		const char* name[4];
	};

	// Table of blend shape names.
	//
	// This table consists of the XRFaceTracker blend shape and
	// the corresponding names (lowercase and no underscore) of:
	// - The Unified Expression blend shape name.
	// - The ARKit blend shape name (if present and different).
	// - The SRanipal blend shape name (if present and different).
	// - The Meta blend shape name (if present and different).
	static constexpr blend_map_entry blend_map[] = {
		{XRTracker::FT_EYE_LOOK_OUT_RIGHT,
			{"eyelookoutright", "eyerightright", "eyeslookoutr"}},
		{XRTracker::FT_EYE_LOOK_IN_RIGHT, {"eyelookinright", "eyerightleft", "eyeslookinr"}},
		{XRTracker::FT_EYE_LOOK_UP_RIGHT, {"eyelookupright", "eyerightlookup", "eyeslookupr"}},
		{XRTracker::FT_EYE_LOOK_DOWN_RIGHT,
			{"eyelookdownright", "eyerightlookdown", "eyeslookdownr"}},
		{XRTracker::FT_EYE_LOOK_OUT_LEFT, {"eyelookoutleft", "eyeleftleft", "eyeslookoutl"}},
		{XRTracker::FT_EYE_LOOK_IN_LEFT, {"eyelookinleft", "eyeleftright", "eyeslookinl"}},
		{XRTracker::FT_EYE_LOOK_UP_LEFT, {"eyelookupleft", "eyeleftlookup", "eyeslookupl"}},
		{XRTracker::FT_EYE_LOOK_DOWN_LEFT,
			{"eyelookdownleft", "eyeleftlookdown", "eyeslookdownl"}},
		{XRTracker::FT_EYE_CLOSED_RIGHT,
			{"eyeclosedright", "eyeblinkright", "eyerightblink", "eyesclosedr"}},
		{XRTracker::FT_EYE_CLOSED_LEFT,
			{"eyeclosedleft", "eyeblinkleft", "eyeleftblink", "eyesclosedl"}},
		{XRTracker::FT_EYE_SQUINT_RIGHT, {"eyesquintright", "eyessquintr"}},
		{XRTracker::FT_EYE_SQUINT_LEFT, {"eyesquintleft", "eyessquintl"}},
		{XRTracker::FT_EYE_WIDE_RIGHT, {"eyewideright", "eyerightwide", "eyeswidenr"}},
		{XRTracker::FT_EYE_WIDE_LEFT, {"eyewideleft", "eyeleftwide", "eyeswidenl"}},
		{XRTracker::FT_EYE_DILATION_RIGHT, {"eyedilationright", "eyerightdilation"}},
		{XRTracker::FT_EYE_DILATION_LEFT, {"eyedilationleft", "eyeleftdilation"}},
		{XRTracker::FT_EYE_CONSTRICT_RIGHT, {"eyeconstrictright", "eyerightconstrict"}},
		{XRTracker::FT_EYE_CONSTRICT_LEFT, {"eyeconstrictleft", "eyeleftconstrict"}},
		{XRTracker::FT_BROW_PINCH_RIGHT, {"browpinchright"}},
		{XRTracker::FT_BROW_PINCH_LEFT, {"browpinchleft"}},
		{XRTracker::FT_BROW_LOWERER_RIGHT, {"browlowererright"}},
		{XRTracker::FT_BROW_LOWERER_LEFT, {"browlowererleft"}},
		{XRTracker::FT_BROW_INNER_UP_RIGHT, {"browinnerupright", "innerbrowraiserr"}},
		{XRTracker::FT_BROW_INNER_UP_LEFT, {"browinnerupleft", "innerbrowraiserl"}},
		{XRTracker::FT_BROW_OUTER_UP_RIGHT, {"browouterupright", "outerbrowraiserr"}},
		{XRTracker::FT_BROW_OUTER_UP_LEFT, {"browouterupleft", "outerbrowraiserl"}},
		{XRTracker::FT_NOSE_SNEER_RIGHT, {"nosesneerright", "nosewrinklerr"}},
		{XRTracker::FT_NOSE_SNEER_LEFT, {"nosesneerleft", "nosewrinklerl"}},
		{XRTracker::FT_NASAL_DILATION_RIGHT, {"nasaldilationright"}},
		{XRTracker::FT_NASAL_DILATION_LEFT, {"nasaldilationleft"}},
		{XRTracker::FT_NASAL_CONSTRICT_RIGHT, {"nasalconstrictright"}},
		{XRTracker::FT_NASAL_CONSTRICT_LEFT, {"nasalconstrictleft"}},
		{XRTracker::FT_CHEEK_SQUINT_RIGHT, {"cheeksquintright", "cheekraiserr"}},
		{XRTracker::FT_CHEEK_SQUINT_LEFT, {"cheeksquintleft", "cheekraiserl"}},
		{XRTracker::FT_CHEEK_PUFF_RIGHT, {"cheekpuffright", "cheekpuffr"}},
		{XRTracker::FT_CHEEK_PUFF_LEFT, {"cheekpuffleft", "cheekpuffl"}},
		{XRTracker::FT_CHEEK_SUCK_RIGHT, {"cheeksuckright", "cheeksuckr"}},
		{XRTracker::FT_CHEEK_SUCK_LEFT, {"cheeksuckleft", "cheeksuckl"}},
		{XRTracker::FT_JAW_OPEN, {"jawopen", "jawdrop"}},
		{XRTracker::FT_MOUTH_CLOSED,
			{"mouthclosed", "mouthclose", "mouthapeshape", "lipstoward"}},
		{XRTracker::FT_JAW_RIGHT, {"jawright", "jawsidewaysright"}},
		{XRTracker::FT_JAW_LEFT, {"jawleft", "jawsidewaysleft"}},
		{XRTracker::FT_JAW_FORWARD, {"jawforward", "jawthrust"}},
		{XRTracker::FT_JAW_BACKWARD, {"jawbackward"}},
		{XRTracker::FT_JAW_CLENCH, {"jawclench"}},
		{XRTracker::FT_JAW_MANDIBLE_RAISE, {"jawmandibleraise"}},
		{XRTracker::FT_LIP_SUCK_UPPER_RIGHT, {"lipsuckupperright", "lipsuckrt"}},
		{XRTracker::FT_LIP_SUCK_UPPER_LEFT, {"lipsuckupperleft", "lipsucklt"}},
		{XRTracker::FT_LIP_SUCK_LOWER_RIGHT, {"lipsucklowerright", "lipsuckrb"}},
		{XRTracker::FT_LIP_SUCK_LOWER_LEFT, {"lipsucklowerleft", "lipsucklb"}},
		{XRTracker::FT_LIP_SUCK_CORNER_RIGHT, {"lipsuckcornerright"}},
		{XRTracker::FT_LIP_SUCK_CORNER_LEFT, {"lipsuckcornerleft"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT, {"lipfunnelupperright", "lipfunnelerrt"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER_LEFT, {"lipfunnelupperleft", "lipfunnelerlt"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT, {"lipfunnellowerright", "lipsuckrb"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER_LEFT, {"lipfunnellowerleft", "lipsucklb"}},
		{XRTracker::FT_LIP_PUCKER_UPPER_RIGHT, {"lippuckerupperright"}},
		{XRTracker::FT_LIP_PUCKER_UPPER_LEFT, {"lippuckerupperleft"}},
		{XRTracker::FT_LIP_PUCKER_LOWER_RIGHT, {"lippuckerlowerright"}},
		{XRTracker::FT_LIP_PUCKER_LOWER_LEFT, {"lippuckerlowerleft"}},
		{XRTracker::FT_MOUTH_UPPER_UP_RIGHT, {"mouthupperupright", "upperlipraiserr"}},
		{XRTracker::FT_MOUTH_UPPER_UP_LEFT, {"mouthupperupleft", "upperlipraiserl"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT,
			{"mouthlowerdownright", "mouthlowerupright", "lowerlipdepressorr"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN_LEFT,
			{"mouthlowerdownleft", "mouthlowerupleft", "lowerlipdepressorl"}},
		{XRTracker::FT_MOUTH_UPPER_DEEPEN_RIGHT, {"mouthupperdeepenright"}},
		{XRTracker::FT_MOUTH_UPPER_DEEPEN_LEFT, {"mouthupperdeepenleft"}},
		{XRTracker::FT_MOUTH_UPPER_RIGHT, {"mouthupperright"}},
		{XRTracker::FT_MOUTH_UPPER_LEFT, {"mouthupperleft"}},
		{XRTracker::FT_MOUTH_LOWER_RIGHT, {"mouthlowerright"}},
		{XRTracker::FT_MOUTH_LOWER_LEFT, {"mouthlowerleft"}},
		{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, {"mouthcornerpullright"}},
		{XRTracker::FT_MOUTH_CORNER_PULL_LEFT, {"mouthcornerpullleft"}},
		{XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT, {"mouthcornerslantright"}},
		{XRTracker::FT_MOUTH_CORNER_SLANT_LEFT, {"mouthcornerslantleft"}},
		{XRTracker::FT_MOUTH_FROWN_RIGHT, {"mouthfrownright", "lipcornerdepressorr"}},
		{XRTracker::FT_MOUTH_FROWN_LEFT, {"mouthfrownleft", "lipcornerdepressorl"}},
		{XRTracker::FT_MOUTH_STRETCH_RIGHT, {"mouthstretchright", "lipstretcherr"}},
		{XRTracker::FT_MOUTH_STRETCH_LEFT, {"mouthstretchleft", "lipstretcherl"}},
		{XRTracker::FT_MOUTH_DIMPLE_RIGHT,
			{"mouthdimplerright", "mouthdimpleright", "dimplerr"}},
		{XRTracker::FT_MOUTH_DIMPLE_LEFT, {"mouthdimplerleft", "mouthdimpleleft", "dimplerl"}},
		{XRTracker::FT_MOUTH_RAISER_UPPER,
			{"mouthraiserupper", "mouthshrugupper", "chinraisert"}},
		{XRTracker::FT_MOUTH_RAISER_LOWER,
			{"mouthraiserlower", "mouthshruglower", "mouthloweroverlay", "chinraiserb"}},
		{XRTracker::FT_MOUTH_PRESS_RIGHT, {"mouthpressright", "lippressorr"}},
		{XRTracker::FT_MOUTH_PRESS_LEFT, {"mouthpressleft", "lippressorl"}},
		{XRTracker::FT_MOUTH_TIGHTENER_RIGHT, {"mouthtightenerright", "liptightenerr"}},
		{XRTracker::FT_MOUTH_TIGHTENER_LEFT, {"mouthtightenerleft", "liptightenerl"}},
		{XRTracker::FT_TONGUE_OUT, {"tongueout", "tonguelongstep2"}},
		{XRTracker::FT_TONGUE_UP, {"tongueup"}},
		{XRTracker::FT_TONGUE_DOWN, {"tonguedown"}},
		{XRTracker::FT_TONGUE_RIGHT, {"tongueright"}},
		{XRTracker::FT_TONGUE_LEFT, {"tongueleft"}},
		{XRTracker::FT_TONGUE_ROLL, {"tongueroll"}},
		{XRTracker::FT_TONGUE_BLEND_DOWN, {"tongueblenddown"}},
		{XRTracker::FT_TONGUE_CURL_UP, {"tonguecurlup"}},
		{XRTracker::FT_TONGUE_SQUISH, {"tonguesquish"}},
		{XRTracker::FT_TONGUE_FLAT, {"tongueflat"}},
		{XRTracker::FT_TONGUE_TWIST_RIGHT, {"tonguetwistright"}},
		{XRTracker::FT_TONGUE_TWIST_LEFT, {"tonguetwistleft"}},
		{XRTracker::FT_SOFT_PALATE_CLOSE, {"softpalateclose"}},
		{XRTracker::FT_THROAT_SWALLOW, {"throatswallow"}},
		{XRTracker::FT_NECK_FLEX_RIGHT, {"neckflexright"}},
		{XRTracker::FT_NECK_FLEX_LEFT, {"neckflexleft"}},
		{XRTracker::FT_EYE_CLOSED, {"eyeclosed"}}, {XRTracker::FT_EYE_WIDE, {"eyewide"}},
		{XRTracker::FT_EYE_SQUINT, {"eyesquint"}},
		{XRTracker::FT_EYE_DILATION, {"eyedilation"}},
		{XRTracker::FT_EYE_CONSTRICT, {"eyeconstrict"}},
		{XRTracker::FT_BROW_DOWN_RIGHT, {"browdownright", "browlowererr"}},
		{XRTracker::FT_BROW_DOWN_LEFT, {"browdownleft", "browlowererl"}},
		{XRTracker::FT_BROW_DOWN, {"browdown"}},
		{XRTracker::FT_BROW_UP_RIGHT, {"browupright"}},
		{XRTracker::FT_BROW_UP_LEFT, {"browupleft"}}, {XRTracker::FT_BROW_UP, {"browup"}},
		{XRTracker::FT_NOSE_SNEER, {"nosesneer"}},
		{XRTracker::FT_NASAL_DILATION, {"nasaldilation"}},
		{XRTracker::FT_NASAL_CONSTRICT, {"nasalconstrict"}},
		{XRTracker::FT_CHEEK_PUFF, {"cheekpuff"}},
		{XRTracker::FT_CHEEK_SUCK, {"cheeksuck"}},
		{XRTracker::FT_CHEEK_SQUINT, {"cheeksquint"}},
		{XRTracker::FT_LIP_SUCK_UPPER, {"lipsuckupper", "mouthrollupper", "mouthupperinside"}},
		{XRTracker::FT_LIP_SUCK_LOWER, {"lipsucklower", "mouthrolllower", "mouthlowerinside"}},
		{XRTracker::FT_LIP_SUCK, {"lipsuck"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER, {"lipfunnelupper", "mouthupperoverturn"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER, {"lipfunnellower", "mouthloweroverturn"}},
		{XRTracker::FT_LIP_FUNNEL, {"lipfunnel", "mouthfunnel"}},
		{XRTracker::FT_LIP_PUCKER_UPPER, {"lippuckerupper"}},
		{XRTracker::FT_LIP_PUCKER_LOWER, {"lippuckerlower"}},
		{XRTracker::FT_LIP_PUCKER, {"lippucker", "mouthpucker", "mouthpout"}},
		{XRTracker::FT_MOUTH_UPPER_UP, {"mouthupperup"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN, {"mouthlowerdown"}},
		{XRTracker::FT_MOUTH_OPEN, {"mouthopen"}},
		{XRTracker::FT_MOUTH_RIGHT, {"mouthright"}},
		{XRTracker::FT_MOUTH_LEFT, {"mouthleft"}},
		{XRTracker::FT_MOUTH_SMILE_RIGHT, {"mouthsmileright", "lipcornerpullerr"}},
		{XRTracker::FT_MOUTH_SMILE_LEFT, {"mouthsmileleft", "lipcornerpullerl"}},
		{XRTracker::FT_MOUTH_SMILE, {"mouthsmile"}},
		{XRTracker::FT_MOUTH_SAD_RIGHT, {"mouthsadright"}},
		{XRTracker::FT_MOUTH_SAD_LEFT, {"mouthsadleft"}},
		{XRTracker::FT_MOUTH_SAD, {"mouthsad"}},
		{XRTracker::FT_MOUTH_STRETCH, {"mouthstretch"}},
		{XRTracker::FT_MOUTH_DIMPLE, {"mouthdimple"}},
		{XRTracker::FT_MOUTH_TIGHTENER, {"mouthtightener"}},
		{XRTracker::FT_MOUTH_PRESS, {"mouthpress"}}};

	// Convert the name to lower-case and strip non-alphanumeric characters.
	const String name = String(p_name).to_lower().remove_char('_');

	// Iterate through the blend map.
	for (const blend_map_entry& entry : blend_map) {
		for (const char* n : entry.name) {
			if (n == nullptr) {
				break;
			}

			if (name == n) {
				return entry.blend;
			}
		}
	}

	// Blend shape not found.
	return -1;
}

// This method adds all the identified XRFaceTracker blend shapes of
// the mesh to the p_blend_mapping map. The map is indexed by the
// XRFaceTracker blend shape, and the value is the index of the mesh
// blend shape.
static void identify_face_blend_shapes(RBMap<int, int>& p_blend_mapping, const Ref<Mesh>& mesh)
{
	// Find all blend shapes.
	const int count = mesh->get_blend_shape_count();
	for (int i = 0; i < count; i++) {
		const int blend = find_face_blend_shape(mesh->get_blend_shape_name(i));
		if (blend >= 0) {
			p_blend_mapping[blend] = i;
		}
	}
}

// This method removes any unified blend shapes from the p_blend_mapping map
// if all the individual blend shapes are found and going to be driven.
static void remove_driven_unified_blend_shapes(RBMap<int, int>& p_blend_mapping)
{
	// Entry for unified blend table.
	struct unified_blend_entry
	{
		int unified;
		int individual[4];
	};

	// Table of unified blend shapes.
	//
	// This table consists of:
	// - The XRFaceTracker unified blend shape.
	// - The individual blend shapes that make up the unified blend shape.
	static constexpr unified_blend_entry unified_blends[] = {
		{XRTracker::FT_EYE_CLOSED,
			{XRTracker::FT_EYE_CLOSED_RIGHT, XRTracker::FT_EYE_CLOSED_LEFT, -1, -1}},
		{XRTracker::FT_EYE_WIDE,
			{XRTracker::FT_EYE_WIDE_RIGHT, XRTracker::FT_EYE_WIDE_LEFT, -1, -1}},
		{XRTracker::FT_EYE_SQUINT,
			{XRTracker::FT_EYE_SQUINT_RIGHT, XRTracker::FT_EYE_SQUINT_LEFT, -1, -1}},
		{XRTracker::FT_EYE_DILATION,
			{XRTracker::FT_EYE_DILATION_RIGHT, XRTracker::FT_EYE_DILATION_LEFT, -1, -1}},
		{XRTracker::FT_EYE_CONSTRICT,
			{XRTracker::FT_EYE_CONSTRICT_RIGHT, XRTracker::FT_EYE_CONSTRICT_LEFT, -1, -1}},
		{XRTracker::FT_BROW_DOWN_RIGHT,
			{XRTracker::FT_BROW_LOWERER_RIGHT, XRTracker::FT_BROW_PINCH_RIGHT, -1, -1}},
		{XRTracker::FT_BROW_DOWN_LEFT,
			{XRTracker::FT_BROW_LOWERER_LEFT, XRTracker::FT_BROW_PINCH_LEFT, -1, -1}},
		{XRTracker::FT_BROW_DOWN,
			{XRTracker::FT_BROW_LOWERER_RIGHT, XRTracker::FT_BROW_PINCH_RIGHT,
				XRTracker::FT_BROW_LOWERER_LEFT, XRTracker::FT_BROW_PINCH_LEFT}},
		{XRTracker::FT_BROW_UP_RIGHT,
			{XRTracker::FT_BROW_INNER_UP_RIGHT, XRTracker::FT_BROW_OUTER_UP_RIGHT, -1, -1}},
		{XRTracker::FT_BROW_UP_LEFT,
			{XRTracker::FT_BROW_INNER_UP_LEFT, XRTracker::FT_BROW_OUTER_UP_LEFT, -1, -1}},
		{XRTracker::FT_BROW_UP,
			{XRTracker::FT_BROW_INNER_UP_RIGHT, XRTracker::FT_BROW_OUTER_UP_RIGHT,
				XRTracker::FT_BROW_INNER_UP_LEFT, XRTracker::FT_BROW_OUTER_UP_LEFT}},
		{XRTracker::FT_NOSE_SNEER,
			{XRTracker::FT_NOSE_SNEER_RIGHT, XRTracker::FT_NOSE_SNEER_LEFT, -1, -1}},
		{XRTracker::FT_NASAL_DILATION, {XRTracker::FT_NASAL_DILATION_RIGHT,
											   XRTracker::FT_NASAL_DILATION_LEFT, -1, -1}},
		{XRTracker::FT_NASAL_CONSTRICT, {XRTracker::FT_NASAL_CONSTRICT_RIGHT,
												XRTracker::FT_NASAL_CONSTRICT_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_PUFF,
			{XRTracker::FT_CHEEK_PUFF_RIGHT, XRTracker::FT_CHEEK_PUFF_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_SUCK,
			{XRTracker::FT_CHEEK_SUCK_RIGHT, XRTracker::FT_CHEEK_SUCK_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_SQUINT,
			{XRTracker::FT_CHEEK_SQUINT_RIGHT, XRTracker::FT_CHEEK_SQUINT_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK_UPPER, {XRTracker::FT_LIP_SUCK_UPPER_RIGHT,
											   XRTracker::FT_LIP_SUCK_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK_LOWER, {XRTracker::FT_LIP_SUCK_LOWER_RIGHT,
											   XRTracker::FT_LIP_SUCK_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK,
			{XRTracker::FT_LIP_SUCK_UPPER_RIGHT, XRTracker::FT_LIP_SUCK_UPPER_LEFT,
				XRTracker::FT_LIP_SUCK_LOWER_RIGHT, XRTracker::FT_LIP_SUCK_LOWER_LEFT}},
		{XRTracker::FT_LIP_FUNNEL_UPPER, {XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT,
												 XRTracker::FT_LIP_FUNNEL_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_FUNNEL_LOWER, {XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT,
												 XRTracker::FT_LIP_FUNNEL_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_FUNNEL,
			{XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT, XRTracker::FT_LIP_FUNNEL_UPPER_LEFT,
				XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT, XRTracker::FT_LIP_FUNNEL_LOWER_LEFT}},
		{XRTracker::FT_LIP_PUCKER_UPPER, {XRTracker::FT_LIP_PUCKER_UPPER_RIGHT,
												 XRTracker::FT_LIP_PUCKER_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_PUCKER_LOWER, {XRTracker::FT_LIP_PUCKER_LOWER_RIGHT,
												 XRTracker::FT_LIP_PUCKER_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_PUCKER,
			{XRTracker::FT_LIP_PUCKER_UPPER_RIGHT, XRTracker::FT_LIP_PUCKER_UPPER_LEFT,
				XRTracker::FT_LIP_PUCKER_LOWER_RIGHT, XRTracker::FT_LIP_PUCKER_LOWER_LEFT}},
		{XRTracker::FT_MOUTH_UPPER_UP, {XRTracker::FT_MOUTH_UPPER_UP_RIGHT,
											   XRTracker::FT_MOUTH_UPPER_UP_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_LOWER_DOWN, {XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT,
												 XRTracker::FT_MOUTH_LOWER_DOWN_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_OPEN,
			{XRTracker::FT_MOUTH_UPPER_UP_RIGHT, XRTracker::FT_MOUTH_UPPER_UP_LEFT,
				XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT, XRTracker::FT_MOUTH_LOWER_DOWN_LEFT}},
		{XRTracker::FT_MOUTH_RIGHT,
			{XRTracker::FT_MOUTH_UPPER_RIGHT, XRTracker::FT_MOUTH_LOWER_RIGHT, -1, -1}},
		{XRTracker::FT_MOUTH_LEFT,
			{XRTracker::FT_MOUTH_UPPER_LEFT, XRTracker::FT_MOUTH_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_SMILE_RIGHT,
			{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT,
				-1, -1}},
		{XRTracker::FT_MOUTH_SMILE_LEFT,
			{XRTracker::FT_MOUTH_CORNER_PULL_LEFT, XRTracker::FT_MOUTH_CORNER_SLANT_LEFT,
				-1, -1}},
		{XRTracker::FT_MOUTH_SMILE,
			{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT,
				XRTracker::FT_MOUTH_CORNER_PULL_LEFT,
				XRTracker::FT_MOUTH_CORNER_SLANT_LEFT}},
		{XRTracker::FT_MOUTH_SAD_RIGHT,
			{XRTracker::FT_MOUTH_FROWN_RIGHT, XRTracker::FT_MOUTH_STRETCH_RIGHT, -1, -1}},
		{XRTracker::FT_MOUTH_SAD_LEFT,
			{XRTracker::FT_MOUTH_FROWN_LEFT, XRTracker::FT_MOUTH_STRETCH_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_SAD,
			{XRTracker::FT_MOUTH_FROWN_RIGHT, XRTracker::FT_MOUTH_STRETCH_RIGHT,
				XRTracker::FT_MOUTH_FROWN_LEFT, XRTracker::FT_MOUTH_STRETCH_LEFT}},
		{XRTracker::FT_MOUTH_STRETCH,
			{XRTracker::FT_MOUTH_STRETCH_RIGHT, XRTracker::FT_MOUTH_STRETCH_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_DIMPLE,
			{XRTracker::FT_MOUTH_DIMPLE_RIGHT, XRTracker::FT_MOUTH_DIMPLE_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_TIGHTENER, {XRTracker::FT_MOUTH_TIGHTENER_RIGHT,
												XRTracker::FT_MOUTH_TIGHTENER_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_PRESS,
			{XRTracker::FT_MOUTH_PRESS_RIGHT, XRTracker::FT_MOUTH_PRESS_LEFT, -1, -1}}};

	// Remove unified blend shapes if individual blend shapes are found.
	for (const unified_blend_entry& entry : unified_blends) {
		// Check if all individual blend shapes are found.
		bool found = true;
		for (const int i : entry.individual) {
			if (i >= 0 && !p_blend_mapping.find(i)) {
				found = false;
				break;
			}
		}

		// If all individual blend shapes are found then remove the unified blend shape.
		if (found) {
			p_blend_mapping.erase(entry.unified);
		}
	}
}


void XRFaceModifier3D::set_face_tracker(const String& p_tracker_name)
{
	tracker_name = p_tracker_name;
	if (is_inside_tree()) {
		_get_blend_data();
	}
}

String XRFaceModifier3D::get_face_tracker() const { return tracker_name; }

void XRFaceModifier3D::set_target(const NodePath& p_target)
{
	target = p_target;

	if (is_inside_tree()) {
		_get_blend_data();
	}
}

NodePath XRFaceModifier3D::get_target() const { return target; }

void XRFaceModifier3D::_get_blend_data()
{
	// This method constructs the blend mapping from the XRFaceTracker
	// blend shapes to the available blend shapes of the target mesh. It does this
	// by:
	//
	// 1. Identifying the blend shapes of the target mesh and identifying what
	//    XRFaceTracker blend shape they correspond to. The results are
	//    placed in the blend_mapping map.
	// 2. Prevent over-driving facial blend-shapes by removing any unified blend
	//    shapes from the map if all the individual blend shapes are already
	//    found and going to be driven.

	blend_mapping.clear();

	// Get the target MeshInstance3D.
	const MeshInstance3D* mesh_instance = get_mesh_instance();
	if (!mesh_instance) {
		return;
	}

	// Get the mesh.
	const Ref<Mesh> mesh = mesh_instance->get_mesh();
	if (mesh.is_null()) {
		return;
	}

	// Identify all face blend shapes and populate the map.
	identify_face_blend_shapes(blend_mapping, mesh);

	// Remove the unified blend shapes if all the individual blend shapes are found.
	remove_driven_unified_blend_shapes(blend_mapping);
}

void XRFaceModifier3D::_update_face_blends() const
{
	// Get the XR Server.
	if (!XRServer::is_initialized()) {
		return;
	}

	// Get the face tracker.
	const Ref<XRTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null() || tracker->get_tracker_type() != XRServer::TRACKER_FACE) {
		return;
	}

	// Get the face mesh.
	MeshInstance3D* mesh_instance = get_mesh_instance();
	if (!mesh_instance) {
		return;
	}

	// Get the blend weights.
	const PackedFloat32Array weights = tracker->get_blend_shapes();

	// Apply all the face blend weights to the mesh.
	for (const KeyValue<int, int>& it : blend_mapping) {
		mesh_instance->set_blend_shape_value(it.value, weights[it.key]);
	}
}

void XRFaceModifier3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_get_blend_data();
		set_process_internal(true);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_process_internal(false);
		blend_mapping.clear();
	} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_face_blends();
	} break;
	default: {
	} break;
	}
}



MeshInstance3D* XRFaceModifier3D::get_mesh_instance() const { return nullptr; }
