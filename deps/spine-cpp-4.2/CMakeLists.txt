cmake_minimum_required (VERSION 3.20)
project(rezoririPlayer-deps)

set(TARGET_NAME "spine-cpp-4.2")

# On Windows, run "dir /b /a-d | clip" in Command Prompt to copy all the filenames in a folder

add_library(${TARGET_NAME} STATIC 
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Animation.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/AnimationState.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/AnimationStateData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Atlas.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/AtlasAttachmentLoader.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Attachment.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/AttachmentLoader.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/AttachmentTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Bone.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/BoneData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/BoundingBoxAttachment.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/ClippingAttachment.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/ColorTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/ConstraintData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/CurveTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/DeformTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/DrawOrderTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Event.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/EventData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/EventTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Extension.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/IkConstraint.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/IkConstraintData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/IkConstraintTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/InheritTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Json.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/LinkedMesh.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Log.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/MathUtil.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/MeshAttachment.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PathAttachment.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PathConstraint.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PathConstraintData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PathConstraintMixTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PathConstraintPositionTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PathConstraintSpacingTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PhysicsConstraint.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PhysicsConstraintData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PhysicsConstraintTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/PointAttachment.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/RegionAttachment.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/RotateTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/RTTI.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/ScaleTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Sequence.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SequenceTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/ShearTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Skeleton.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SkeletonBinary.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SkeletonBounds.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SkeletonClipping.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SkeletonData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SkeletonJson.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SkeletonRenderer.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Skin.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Slot.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SlotData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/SpineObject.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/TextureLoader.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Timeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/TransformConstraint.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/TransformConstraintData.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/TransformConstraintTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/TranslateTimeline.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Triangulator.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/Updatable.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/spine/VertexAttachment.cpp"
)

target_include_directories(${TARGET_NAME} PRIVATE include)

set_target_properties(${TARGET_NAME} PROPERTIES
	DEBUG_POSTFIX "-d"
	ARCHIVE_OUTPUT_NAME ${TARGET_NAME}
)

if(MSVC)
	target_compile_definitions(${TARGET_NAME} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/d1trimfile:${CMAKE_SOURCE_DIR}\\>")
	target_link_options(${TARGET_NAME} PRIVATE "/pdbaltpath:%_PDB%")
endif()
