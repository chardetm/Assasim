/**
 * \file error_detection.hpp
 * \brief Contains all the error handling over models.
 */

#ifndef ERROR_DETECTION_HPP_
#define ERROR_DETECTION_HPP_

#include "model.hpp"

/// Checks if an Interaction contains methods or private attributes.
void IsThereMethodOrPrivateAttributesInInteraction(Model &model);

/// Checks that all public attributes of Interaction and Agent classes are of structural type.
void ArePublicAttributsOfStruturalTypeInInteractionOrAgent(Model &model, clang::ASTContext *context);

/// Checks if an Agent has a private attribute of non structural type (and set it to unsendable).
void AreThereAPrivateAttributesOfNonStructuralType(Model &model, clang::ASTContext *context);

/// Checks if an attribute is defined as static.
void DoesAnAgentContainsAnAttributeDefinedAsStatic(Model &model);

/// Checks if an attribute of an Agent is defined as private and critical.
void IsAnAttributeOfAnAgentDefinedAsPrivateAndCritical(Model &model);

/// Checks if an agent or interaction contains an anonymous struct, which leads to an error.
void IsThereAnAnonymousStructInAttributes(Model &model);

/// Checks the presence of the errors detected in the functions  in the model.
void CheckErrorInModel(Model &model, clang::ASTContext *context);

#endif
