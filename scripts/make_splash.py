#!/usr/bin/env python3

import os
import shutil
import subprocess
import tempfile
import uuid
from collections import OrderedDict
from pathlib import Path

import methods

EXPORT_ICONS = [
    "platform/android/export/logo.svg",
    "platform/android/export/run_icon.svg",
    "platform/ios/export/logo.svg",
    "platform/ios/export/run_icon.svg",
    "platform/linuxbsd/export/logo.svg",
    "platform/linuxbsd/export/run_icon.svg",
    "platform/macos/export/logo.svg",
    "platform/macos/export/run_icon.svg",
    "platform/visionos/export/logo.svg",
    "platform/visionos/export/run_icon.svg",
    "platform/web/export/logo.svg",
    "platform/web/export/run_icon.svg",
    "platform/windows/export/logo.svg",
    "platform/windows/export/run_icon.svg",
]

THEME_ICONS = [
    "scene/theme/icons/action_copy.svg", "scene/theme/icons/add.svg", "scene/theme/icons/arrow_down.svg",
    "scene/theme/icons/arrow_left.svg", "scene/theme/icons/arrow_right.svg", "scene/theme/icons/arrow_up.svg",
    "scene/theme/icons/bookmark.svg", "scene/theme/icons/breakpoint.svg", "scene/theme/icons/checked.svg",
    "scene/theme/icons/checked_disabled.svg", "scene/theme/icons/clear.svg", "scene/theme/icons/close.svg",
    "scene/theme/icons/close_hl.svg", "scene/theme/icons/color_picker_bar_arrow.svg", "scene/theme/icons/color_picker_cursor.svg",
    "scene/theme/icons/color_picker_cursor_bg.svg", "scene/theme/icons/color_picker_overbright.svg", "scene/theme/icons/color_picker_pipette.svg",
    "scene/theme/icons/error_icon.svg", "scene/theme/icons/favorite.svg", "scene/theme/icons/file.svg",
    "scene/theme/icons/file_mode_list.svg", "scene/theme/icons/file_mode_thumbnail.svg", "scene/theme/icons/file_thumbnail.svg",
    "scene/theme/icons/folder.svg", "scene/theme/icons/folder_create.svg", "scene/theme/icons/folder_thumbnail.svg",
    "scene/theme/icons/folder_up.svg", "scene/theme/icons/graph_port.svg", "scene/theme/icons/grid_layout.svg",
    "scene/theme/icons/grid_minimap.svg", "scene/theme/icons/grid_snap.svg", "scene/theme/icons/grid_toggle.svg",
    "scene/theme/icons/h_dragger.svg", "scene/theme/icons/hslider_tick.svg", "scene/theme/icons/hsplitter.svg",
    "scene/theme/icons/indeterminate.svg", "scene/theme/icons/indeterminate_disabled.svg", "scene/theme/icons/line_edit_clear.svg",
    "scene/theme/icons/load.svg", "scene/theme/icons/mini_checkerboard.svg", "scene/theme/icons/move_down.svg",
    "scene/theme/icons/move_up.svg", "scene/theme/icons/option_button_arrow.svg", "scene/theme/icons/picker_shape_circle.svg",
    "scene/theme/icons/picker_shape_rectangle.svg", "scene/theme/icons/picker_shape_rectangle_wheel.svg", "scene/theme/icons/popup_menu_arrow_left.svg",
    "scene/theme/icons/popup_menu_arrow_right.svg", "scene/theme/icons/radio_checked.svg", "scene/theme/icons/radio_checked_disabled.svg",
    "scene/theme/icons/radio_unchecked.svg", "scene/theme/icons/radio_unchecked_disabled.svg", "scene/theme/icons/region_folded.svg",
    "scene/theme/icons/region_unfolded.svg", "scene/theme/icons/reload.svg", "scene/theme/icons/resizer_nw.svg",
    "scene/theme/icons/resizer_se.svg", "scene/theme/icons/save.svg", "scene/theme/icons/script.svg",
    "scene/theme/icons/scroll_button_left.svg", "scene/theme/icons/scroll_button_left_hl.svg", "scene/theme/icons/scroll_button_right.svg",
    "scene/theme/icons/scroll_button_right_hl.svg", "scene/theme/icons/scroll_hint_horizontal.svg", "scene/theme/icons/scroll_hint_vertical.svg",
    "scene/theme/icons/search.svg", "scene/theme/icons/slider_grabber.svg", "scene/theme/icons/slider_grabber_disabled.svg",
    "scene/theme/icons/slider_grabber_hl.svg", "scene/theme/icons/sort.svg", "scene/theme/icons/tabs_drop_mark.svg",
    "scene/theme/icons/tabs_menu.svg", "scene/theme/icons/tabs_menu_hl.svg", "scene/theme/icons/text_edit_ellipsis.svg",
    "scene/theme/icons/text_edit_space.svg", "scene/theme/icons/text_edit_tab.svg", "scene/theme/icons/toggle_filename_filter.svg",
    "scene/theme/icons/toggle_off.svg", "scene/theme/icons/toggle_off_disabled.svg", "scene/theme/icons/toggle_off_disabled_mirrored.svg",
    "scene/theme/icons/toggle_off_mirrored.svg", "scene/theme/icons/toggle_on.svg", "scene/theme/icons/toggle_on_disabled.svg",
    "scene/theme/icons/toggle_on_disabled_mirrored.svg", "scene/theme/icons/toggle_on_mirrored.svg", "scene/theme/icons/unchecked.svg",
    "scene/theme/icons/unchecked_disabled.svg", "scene/theme/icons/updown.svg", "scene/theme/icons/v_dragger.svg",
    "scene/theme/icons/value_down.svg", "scene/theme/icons/value_up.svg", "scene/theme/icons/visibility_visible.svg",
    "scene/theme/icons/vslider_tick.svg", "scene/theme/icons/vsplitter.svg", "scene/theme/icons/zoom_less.svg",
    "scene/theme/icons/zoom_more.svg", "scene/theme/icons/zoom_reset.svg",
]

BUILTIN_FONTS = [
    "thirdparty/fonts/DroidSansFallback.woff2", "thirdparty/fonts/DroidSansJapanese.woff2",
    "thirdparty/fonts/Inter_Bold.woff2", "thirdparty/fonts/Inter_Regular.woff2",
    "thirdparty/fonts/JetBrainsMono_Regular.woff2", "thirdparty/fonts/NotoSansBengali_Bold.woff2",
    "thirdparty/fonts/NotoSansBengali_Regular.woff2", "thirdparty/fonts/NotoSansDevanagari_Bold.woff2",
    "thirdparty/fonts/NotoSansDevanagari_Regular.woff2", "thirdparty/fonts/NotoSansGeorgian_Bold.woff2",
    "thirdparty/fonts/NotoSansGeorgian_Regular.woff2", "thirdparty/fonts/NotoSansHebrew_Bold.woff2",
    "thirdparty/fonts/NotoSansHebrew_Regular.woff2", "thirdparty/fonts/NotoSansMalayalamUI_Bold.woff2",
    "thirdparty/fonts/NotoSansMalayalamUI_Regular.woff2", "thirdparty/fonts/NotoSansOriya_Bold.woff2",
    "thirdparty/fonts/NotoSansOriya_Regular.woff2", "thirdparty/fonts/NotoSansSinhala_Bold.woff2",
    "thirdparty/fonts/NotoSansSinhala_Regular.woff2", "thirdparty/fonts/NotoSansTamilUI_Bold.woff2",
    "thirdparty/fonts/NotoSansTamilUI_Regular.woff2", "thirdparty/fonts/NotoSansTeluguUI_Bold.woff2",
    "thirdparty/fonts/NotoSansTeluguUI_Regular.woff2", "thirdparty/fonts/NotoSansThai_Bold.woff2",
    "thirdparty/fonts/NotoSansThai_Regular.woff2", "thirdparty/fonts/Vazirmatn_Bold.woff2",
    "thirdparty/fonts/Vazirmatn_Regular.woff2",
]

DOC_CLASS_PATHS = {'EditorExportPlatformAndroid': 'platform/android/doc_classes', 'EditorExportPlatformIOS': 'platform/ios/doc_classes', 'EditorExportPlatformLinuxBSD': 'platform/linuxbsd/doc_classes', 'EditorExportPlatformMacOS': 'platform/macos/doc_classes', 'EditorExportPlatformVisionOS': 'platform/visionos/doc_classes', 'EditorExportPlatformWeb': 'platform/web/doc_classes', 'EditorExportPlatformWindows': 'platform/windows/doc_classes', 'CSGBox3D': 'modules/csg/doc_classes', 'CSGCombiner3D': 'modules/csg/doc_classes', 'CSGCylinder3D': 'modules/csg/doc_classes', 'CSGMesh3D': 'modules/csg/doc_classes', 'CSGPolygon3D': 'modules/csg/doc_classes', 'CSGPrimitive3D': 'modules/csg/doc_classes', 'CSGShape3D': 'modules/csg/doc_classes', 'CSGSphere3D': 'modules/csg/doc_classes', 'CSGTorus3D': 'modules/csg/doc_classes', 'ENetMultiplayerPeer': 'modules/enet/doc_classes', 'ENetConnection': 'modules/enet/doc_classes', 'ENetPacketPeer': 'modules/enet/doc_classes', 'EditorSceneFormatImporterFBX2GLTF': 'modules/fbx/doc_classes', 'EditorSceneFormatImporterUFBX': 'modules/fbx/doc_classes', 'FBXDocument': 'modules/fbx/doc_classes', 'FBXState': 'modules/fbx/doc_classes', 'EditorSceneFormatImporterBlend': 'modules/gltf/doc_classes', 'EditorSceneFormatImporterGLTF': 'modules/gltf/doc_classes', 'GLTFAccessor': 'modules/gltf/doc_classes', 'GLTFAnimation': 'modules/gltf/doc_classes', 'GLTFBufferView': 'modules/gltf/doc_classes', 'GLTFCamera': 'modules/gltf/doc_classes', 'GLTFDocument': 'modules/gltf/doc_classes', 'GLTFDocumentExtension': 'modules/gltf/doc_classes', 'GLTFDocumentExtensionConvertImporterMesh': 'modules/gltf/doc_classes', 'GLTFLight': 'modules/gltf/doc_classes', 'GLTFMesh': 'modules/gltf/doc_classes', 'GLTFNode': 'modules/gltf/doc_classes', 'GLTFObjectModelProperty': 'modules/gltf/doc_classes', 'GLTFPhysicsBody': 'modules/gltf/doc_classes', 'GLTFPhysicsShape': 'modules/gltf/doc_classes', 'GLTFSkeleton': 'modules/gltf/doc_classes', 'GLTFSkin': 'modules/gltf/doc_classes', 'GLTFSpecGloss': 'modules/gltf/doc_classes', 'GLTFState': 'modules/gltf/doc_classes', 'GLTFTexture': 'modules/gltf/doc_classes', 'GLTFTextureSampler': 'modules/gltf/doc_classes', 'GridMap': 'modules/gridmap/doc_classes', 'GridMapEditorPlugin': 'modules/gridmap/doc_classes', 'AudioStreamPlaylist': 'modules/interactive_music/doc_classes', 'AudioStreamPlaybackPlaylist': 'modules/interactive_music/doc_classes', 'AudioStreamInteractive': 'modules/interactive_music/doc_classes', 'AudioStreamPlaybackInteractive': 'modules/interactive_music/doc_classes', 'AudioStreamSynchronized': 'modules/interactive_music/doc_classes', 'AudioStreamPlaybackSynchronized': 'modules/interactive_music/doc_classes', 'MobileVRInterface': 'modules/mobile_vr/doc_classes', 'AudioStreamMP3': 'modules/mp3/doc_classes', 'ResourceImporterMP3': 'modules/mp3/doc_classes', 'SceneReplicationConfig': 'modules/multiplayer/doc_classes', 'SceneMultiplayer': 'modules/multiplayer/doc_classes', 'MultiplayerSpawner': 'modules/multiplayer/doc_classes', 'MultiplayerSynchronizer': 'modules/multiplayer/doc_classes', 'OfflineMultiplayerPeer': 'modules/multiplayer/doc_classes', 'FastNoiseLite': 'modules/noise/doc_classes', 'Noise': 'modules/noise/doc_classes', 'NoiseTexture2D': 'modules/noise/doc_classes', 'NoiseTexture3D': 'modules/noise/doc_classes', 'OggPacketSequence': 'modules/ogg/doc_classes', 'OggPacketSequencePlayback': 'modules/ogg/doc_classes', 'OpenXRInterface': 'modules/openxr/doc_classes', 'OpenXRAction': 'modules/openxr/doc_classes', 'OpenXRActionSet': 'modules/openxr/doc_classes', 'OpenXRActionMap': 'modules/openxr/doc_classes', 'OpenXRAPIExtension': 'modules/openxr/doc_classes', 'OpenXRExtensionWrapper': 'modules/openxr/doc_classes', 'OpenXRExtensionWrapperExtension': 'modules/openxr/doc_classes', 'OpenXRFrameSynthesisExtension': 'modules/openxr/doc_classes', 'OpenXRFutureResult': 'modules/openxr/doc_classes', 'OpenXRFutureExtension': 'modules/openxr/doc_classes', 'OpenXRInteractionProfile': 'modules/openxr/doc_classes', 'OpenXRInteractionProfileMetadata': 'modules/openxr/doc_classes', 'OpenXRIPBinding': 'modules/openxr/doc_classes', 'OpenXRHand': 'modules/openxr/doc_classes', 'OpenXRVisibilityMask': 'modules/openxr/doc_classes', 'OpenXRCompositionLayer': 'modules/openxr/doc_classes', 'OpenXRCompositionLayerQuad': 'modules/openxr/doc_classes', 'OpenXRCompositionLayerCylinder': 'modules/openxr/doc_classes', 'OpenXRCompositionLayerEquirect': 'modules/openxr/doc_classes', 'OpenXRBindingModifier': 'modules/openxr/doc_classes', 'OpenXRIPBindingModifier': 'modules/openxr/doc_classes', 'OpenXRActionBindingModifier': 'modules/openxr/doc_classes', 'OpenXRAnalogThresholdModifier': 'modules/openxr/doc_classes', 'OpenXRDpadBindingModifier': 'modules/openxr/doc_classes', 'OpenXRInteractionProfileEditorBase': 'modules/openxr/doc_classes', 'OpenXRInteractionProfileEditor': 'modules/openxr/doc_classes', 'OpenXRBindingModifierEditor': 'modules/openxr/doc_classes', 'OpenXRHapticBase': 'modules/openxr/doc_classes', 'OpenXRHapticVibration': 'modules/openxr/doc_classes', 'OpenXRRenderModelExtension': 'modules/openxr/doc_classes', 'OpenXRRenderModel': 'modules/openxr/doc_classes', 'OpenXRRenderModelManager': 'modules/openxr/doc_classes', 'OpenXRStructureBase': 'modules/openxr/doc_classes', 'OpenXRSpatialEntityExtension': 'modules/openxr/doc_classes', 'OpenXRSpatialEntityTracker': 'modules/openxr/doc_classes', 'OpenXRAnchorTracker': 'modules/openxr/doc_classes', 'OpenXRPlaneTracker': 'modules/openxr/doc_classes', 'OpenXRMarkerTracker': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationBaseHeader': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationAnchor': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationQrCode': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationMicroQrCode': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationAruco': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationAprilTag': 'modules/openxr/doc_classes', 'OpenXRSpatialContextPersistenceConfig': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationPlaneTracking': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentData': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentBounded2DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentBounded3DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentParentList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentMesh2DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentMesh3DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentPlaneAlignmentList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentPolygon2DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentPlaneSemanticLabelList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentMarkerList': 'modules/openxr/doc_classes', 'OpenXRSpatialQueryResultData': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentAnchorList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentPersistenceList': 'modules/openxr/doc_classes', 'OpenXRSpatialAnchorCapability': 'modules/openxr/doc_classes', 'OpenXRSpatialPlaneTrackingCapability': 'modules/openxr/doc_classes', 'OpenXRSpatialMarkerTrackingCapability': 'modules/openxr/doc_classes', 'OpenXRAndroidThreadSettingsExtension': 'modules/openxr/doc_classes', 'OpenXRUserPresenceExtension': 'modules/openxr/doc_classes', 'RegEx': 'modules/regex/doc_classes', 'RegExMatch': 'modules/regex/doc_classes', 'TextServerAdvanced': 'modules/text_server_adv/doc_classes', 'VideoStreamTheora': 'modules/theora/doc_classes', 'UPNP': 'modules/upnp/doc_classes', 'UPNPDevice': 'modules/upnp/doc_classes', 'VisualShader': 'modules/visual_shader/doc_classes', 'VisualShaderNode': 'modules/visual_shader/doc_classes', 'VisualShaderNodeBillboard': 'modules/visual_shader/doc_classes', 'VisualShaderNodeBooleanConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeBooleanParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeClamp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeColorConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeColorFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeColorOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeColorParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeComment': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCompare': 'modules/visual_shader/doc_classes', 'VisualShaderNodeConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCubemap': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCubemapParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCurveTexture': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCurveXYZTexture': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCustom': 'modules/visual_shader/doc_classes', 'VisualShaderNodeDerivativeFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeDeterminant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeDistanceFade': 'modules/visual_shader/doc_classes', 'VisualShaderNodeDotProduct': 'modules/visual_shader/doc_classes', 'VisualShaderNodeExpression': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFaceForward': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFloatConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFloatFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFloatOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFloatParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFrame': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFresnel': 'modules/visual_shader/doc_classes', 'VisualShaderNodeGlobalExpression': 'modules/visual_shader/doc_classes', 'VisualShaderNodeGroupBase': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIf': 'modules/visual_shader/doc_classes', 'VisualShaderNodeInput': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIntConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIntFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIntOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIntParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIs': 'modules/visual_shader/doc_classes', 'VisualShaderNodeLinearSceneDepth': 'modules/visual_shader/doc_classes', 'VisualShaderNodeMix': 'modules/visual_shader/doc_classes', 'VisualShaderNodeMultiplyAdd': 'modules/visual_shader/doc_classes', 'VisualShaderNodeOuterProduct': 'modules/visual_shader/doc_classes', 'VisualShaderNodeOutput': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParameterRef': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleAccelerator': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleBoxEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleConeVelocity': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleEmit': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleMeshEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleMultiplyByAxisAngle': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleOutput': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleRandomness': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleRingEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleSphereEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeProximityFade': 'modules/visual_shader/doc_classes', 'VisualShaderNodeRandomRange': 'modules/visual_shader/doc_classes', 'VisualShaderNodeRemap': 'modules/visual_shader/doc_classes', 'VisualShaderNodeReroute': 'modules/visual_shader/doc_classes', 'VisualShaderNodeResizableBase': 'modules/visual_shader/doc_classes', 'VisualShaderNodeRotationByAxis': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSample3D': 'modules/visual_shader/doc_classes', 'VisualShaderNodeScreenNormalWorldSpace': 'modules/visual_shader/doc_classes', 'VisualShaderNodeScreenUVToSDF': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSDFRaymarch': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSDFToScreenUV': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSmoothStep': 'modules/visual_shader/doc_classes', 'VisualShaderNodeStep': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSwitch': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture2DArray': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture2DArrayParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture2DParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture3D': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture3DParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTextureParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTextureParameterTriplanar': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTextureSDF': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTextureSDFNormal': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformCompose': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformDecompose': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformVecMult': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUIntConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUIntFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUIntOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUIntParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUVFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUVPolarCoord': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVarying': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVaryingGetter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVaryingSetter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec2Constant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec2Parameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec3Constant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec3Parameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec4Constant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec4Parameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorBase': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorCompose': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorDecompose': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorDistance': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorLen': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorRefract': 'modules/visual_shader/doc_classes', 'VisualShaderNodeWorldPositionFromDepth': 'modules/visual_shader/doc_classes', 'AudioStreamOggVorbis': 'modules/vorbis/doc_classes', 'AudioStreamPlaybackOggVorbis': 'modules/vorbis/doc_classes', 'ResourceImporterOggVorbis': 'modules/vorbis/doc_classes', 'WebRTCPeerConnection': 'modules/webrtc/doc_classes', 'WebRTCDataChannel': 'modules/webrtc/doc_classes', 'WebRTCMultiplayerPeer': 'modules/webrtc/doc_classes', 'WebRTCPeerConnectionExtension': 'modules/webrtc/doc_classes', 'WebRTCDataChannelExtension': 'modules/webrtc/doc_classes', 'WebSocketClient': 'modules/websocket/doc_classes', 'WebSocketMultiplayerPeer': 'modules/websocket/doc_classes', 'WebSocketPeer': 'modules/websocket/doc_classes', 'WebSocketServer': 'modules/websocket/doc_classes', 'WebXRInterface': 'modules/webxr/doc_classes', 'ZIPReader': 'modules/zip/doc_classes', 'ZIPPacker': 'modules/zip/doc_classes'}

EXTRACTABLE_SOURCES = [
    "editor/translations/extractable/ar.po", "editor/translations/extractable/bg.po",
    "editor/translations/extractable/ca.po", "editor/translations/extractable/cs.po",
    "editor/translations/extractable/de.po", "editor/translations/extractable/el.po",
    "editor/translations/extractable/es.po", "editor/translations/extractable/es_AR.po",
    "editor/translations/extractable/et.po", "editor/translations/extractable/extractable.pot",
    "editor/translations/extractable/fa.po", "editor/translations/extractable/fi.po",
    "editor/translations/extractable/fr.po", "editor/translations/extractable/he.po",
    "editor/translations/extractable/id.po", "editor/translations/extractable/it.po",
    "editor/translations/extractable/ja.po", "editor/translations/extractable/ko.po",
    "editor/translations/extractable/lv.po", "editor/translations/extractable/nl.po",
    "editor/translations/extractable/pl.po", "editor/translations/extractable/pt.po",
    "editor/translations/extractable/pt_BR.po", "editor/translations/extractable/ru.po",
    "editor/translations/extractable/sk.po", "editor/translations/extractable/sv.po",
    "editor/translations/extractable/th.po", "editor/translations/extractable/tr.po",
    "editor/translations/extractable/uk.po", "editor/translations/extractable/vi.po",
    "editor/translations/extractable/zh_Hans.po", "editor/translations/extractable/zh_Hant.po",
]


def _compile_extractable_entries(sources):
    valid_sources = [src for src in sources if os.path.isfile(src)]
    sorted_paths = sorted(
        [os.path.abspath(src) for src in valid_sources],
        key=lambda path: os.path.splitext(os.path.basename(path))[0],
    )
    xl_names = []
    blobs = []
    msgfmt = shutil.which("msgfmt")
    for path in sorted_paths:
        name = os.path.splitext(os.path.basename(path))[0]
        if msgfmt and name != "extractable":
            mo_path = os.path.join(tempfile.gettempdir(), uuid.uuid4().hex + ".mo")
            try:
                subprocess.run([msgfmt, path, "--no-hash", "-o", mo_path], stderr=subprocess.PIPE, check=False)
                buffer = methods.get_buffer(mo_path) if os.path.exists(mo_path) else methods.get_buffer(path)
            except OSError:
                buffer = methods.get_buffer(path)
            finally:
                if os.path.exists(mo_path):
                    try:
                        os.remove(mo_path)
                    except OSError:
                        pass
        else:
            buffer = methods.get_buffer(path)
            if name == "extractable":
                name = "source"
        decomp_size = len(buffer)
        buffer = methods.compress_buffer(buffer)
        blobs.append(
            f"inline constexpr const unsigned char _extractable_translation_{name}_compressed[] = {{\n"
            f"{methods.format_buffer(buffer, 1)}\n}};\n\n"
        )
        xl_names.append([name, len(buffer), decomp_size])
    return blobs, xl_names


def make_splash_and_icons(target, splash_src, icon_src):
    splash_buf = methods.get_buffer(str(splash_src))
    icon_buf = methods.get_buffer(str(icon_src))
    font_buf = methods.get_buffer("thirdparty/fonts/OpenSans_SemiBold.woff2")

    icons_names = []
    icons_raw = []
    for src in THEME_ICONS:
        with open(src, encoding="utf-8", newline="\n") as f:
            icons_raw.append(methods.to_raw_cstring(f.read()))
        icons_names.append(f'"{os.path.splitext(os.path.basename(src))[0]}"')

    icons_names_str = ",\n\t".join(icons_names)
    icons_raw_str = ",\n\t".join(icons_raw)

    doc_paths = dict(sorted((k, v) for k, v in DOC_CLASS_PATHS.items() if os.path.isdir(v)))
    doc_data = "\n".join([f'\t{{"{key}", "{value}"}},' for key, value in doc_paths.items()])

    platforms = ["android", "ios", "visionos", "web"]
    api_reg = "\n\t".join([f"register_{p}_api();" for p in platforms])
    api_unreg = "\n\t".join([f"unregister_{p}_api();" for p in platforms])

    modules = OrderedDict(methods.detect_modules("modules", recursive=False).items())
    decls = []
    for key in modules.keys():
        if key not in ("freetype", "msdfgen"):
            decls.append(f"void initialize_{key}_module(ModuleInitializationLevel p_level);")
            decls.append(f"void uninitialize_{key}_module(ModuleInitializationLevel p_level);")
    mod_inc = "\n".join(decls)
    mod_init = "\n".join([f"#ifdef MODULE_{key.upper()}_ENABLED\n\tinitialize_{key}_module(p_level);\n#endif" for key in modules.keys()])
    mod_uninit = "\n".join([f"#ifdef MODULE_{key.upper()}_ENABLED\n\tuninitialize_{key}_module(p_level);\n#endif" for key in modules.keys()])

    exp_platforms = ["android", "ios", "linuxbsd", "macos", "visionos", "web", "windows"]
    exp_reg = "\n\t".join([f"register_{p}_exporter();" for p in exp_platforms])
    exp_type = "\n\t".join([f"register_{p}_exporter_types();" for p in exp_platforms])
    ext_blobs, ext_xl_names = _compile_extractable_entries(EXTRACTABLE_SOURCES)

    with open(str(target), "w", encoding="utf-8", newline="\n") as out:
        out.write("/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n\n")
        out.write("#ifndef MAIN_SPLASH_GEN_H\n#define MAIN_SPLASH_GEN_H\n\n")
        out.write(f"""\
#include "core/math/vector2.h"

inline const Color boot_splash_bg_color = Color(0.14, 0.14, 0.14);
inline constexpr const unsigned char boot_splash_png[] = {{
{methods.format_buffer(splash_buf, 1)}
}};

inline constexpr const unsigned char app_icon_png[] = {{
{methods.format_buffer(icon_buf, 1)}
}};

#ifndef _FONT_OPENSANS_SEMIBOLD_DEFINED
#define _FONT_OPENSANS_SEMIBOLD_DEFINED
inline constexpr int _font_OpenSans_SemiBold_size = {len(font_buf)};
inline constexpr unsigned char _font_OpenSans_SemiBold[] = {{
	{methods.format_buffer(font_buf, 1)}
}};
#endif // _FONT_OPENSANS_SEMIBOLD_DEFINED

inline constexpr int default_theme_icons_count = {len(icons_names)};
inline constexpr const char *default_theme_icons_sources[] = {{
	{icons_raw_str}
}};

inline constexpr const char *default_theme_icons_names[] = {{
	{icons_names_str}
}};

""")
        for src in EXPORT_ICONS:
            src_path = Path(src)
            src_name = src_path.stem
            platform = src_path.parent.parent.stem
            with open(src, "r", encoding="utf-8") as f:
                svg = f.read()
            out.write(f"inline constexpr const char *_{platform}_{src_name}_svg = {methods.to_raw_cstring(svg)};\n")

        out.write(f"""
struct _DocDataClassPath {{
	const char *name;
	const char *path;
}};

inline constexpr int _doc_data_class_path_count = {len(doc_paths)};
inline constexpr _DocDataClassPath _doc_data_class_paths[{len(doc_paths) + 1}] = {{
{doc_data}
	{{nullptr, nullptr}},
}};

""")
        for fsrc in BUILTIN_FONTS:
            fbuf = methods.get_buffer(fsrc)
            fname = os.path.splitext(os.path.basename(fsrc))[0]
            out.write(
                f"inline constexpr int _font_{fname}_size = {len(fbuf)};\n"
                f"inline constexpr unsigned char _font_{fname}[] = {{\n\t{methods.format_buffer(fbuf, 1)}\n}};\n\n"
            )

        out.write("""\
#ifndef EDITOR_TRANSLATION_LIST
#define EDITOR_TRANSLATION_LIST

struct EditorTranslationList {
	const char* lang;
	int comp_size;
	int uncomp_size;
	const unsigned char* data;
};

#endif // EDITOR_TRANSLATION_LIST

extern const EditorTranslationList _extractable_translations[];
extern const EditorTranslationList _doc_translations[];

#endif // MAIN_SPLASH_GEN_H

""")
        out.write(f"""\
#ifdef DEFINE_MAIN_GENERATED_REGISTRIES
#undef DEFINE_MAIN_GENERATED_REGISTRIES

#include "core/version_generated.gen.h"

void register_platform_apis() {{
	{api_reg}
}}

void unregister_platform_apis() {{
	{api_unreg}
}}

{mod_inc}

void initialize_modules(ModuleInitializationLevel p_level) {{
{mod_init}
}}

void uninitialize_modules(ModuleInitializationLevel p_level) {{
{mod_uninit}
}}

#endif // DEFINE_MAIN_GENERATED_REGISTRIES

#ifdef DEFINE_EDITOR_EXPORTERS_AND_TRANSLATIONS
#undef DEFINE_EDITOR_EXPORTERS_AND_TRANSLATIONS

void register_exporters() {{
	{exp_reg}
}}

void register_exporter_types() {{
	{exp_type}
}}

const EditorTranslationList _doc_translations[] = {{
	{{ nullptr, 0, 0, nullptr }},
}};

""")
        for b in ext_blobs:
            out.write(b)
        out.write("const EditorTranslationList _extractable_translations[] = {\n")
        for x in ext_xl_names:
            out.write(f'\t{{ "{x[0]}", {x[1]}, {x[2]}, _extractable_translation_{x[0]}_compressed }},\n')
        out.write("\t{ nullptr, 0, 0, nullptr },\n};\n\n#endif // DEFINE_EDITOR_EXPORTERS_AND_TRANSLATIONS\n")


if __name__ == "__main__":
    make_splash_and_icons("main/splash.gen.h", "main/splash.png", "main/app_icon.png")
