#include "error_detection.hpp"
#include "analyze_class.hpp"


void IsThereMethodOrPrivateAttributesInInteraction(Model &model) {
    for (const auto &interaction : model.GetInteractions()) {
        for (const auto &field : interaction.second.GetFields()) {
            if (field.second.GetAccess() == clang::AS_private) {
                clang::FullSourceLoc loc = clang::FullSourceLoc(interaction.second.GetDecl()->getLocStart(),
                                                                *model.GetSourceManager());
                ErrorMessage(loc) << "in Interaction " << interaction.first << ", field "
                                    << field.first << " defined as private";
                model.AddErrorFound();
            }
        }
		size_t user_provided_methods = std::distance(interaction.second.GetDecl()->method_begin(), interaction.second.GetDecl()->method_end())-std::distance(interaction.second.GetDecl()->ctor_begin(),interaction.second.GetDecl()->ctor_end());
		
        if (user_provided_methods > 1) {
            clang::FullSourceLoc loc = clang::FullSourceLoc(interaction.second.GetDecl()->getLocStart(),
                                                            *model.GetSourceManager());
            ErrorMessage(loc) << "in interaction " << interaction.first
							  << ", user specified methods are not allowed in Interaction and there are "
							  << user_provided_methods
							  << " methods";
            model.AddErrorFound();
        }
    }
}


void ArePublicAttributsOfStruturalTypeInInteractionOrAgent(Model &model, clang::ASTContext *context) {
    for (const auto &interaction : model.GetInteractions()) {
        for (const auto &field : interaction.second.GetFields()) {
            if ((field.second.GetAccess() == clang::AS_public) &&
                (!IsStructuralType(field.second.GetType(), context))) {
                clang::FullSourceLoc loc = clang::FullSourceLoc(interaction.second.GetDecl()->getLocStart(),
                                                                *model.GetSourceManager());
                ErrorMessage(loc) << "in Interaction " << interaction.first << ", public attribute "
                                    << field.first << " is not of structural type (1)";

                model.AddErrorFound();
            }
        }
    }
    for (const auto &agent : model.GetAgents()) {
        for (const auto &field : agent.second.GetFields()) {
            if ((field.second.GetAccess() == clang::AS_public) &&
                (!IsStructuralType(field.second.GetType(), context))) {
                clang::FullSourceLoc loc = clang::FullSourceLoc(agent.second.GetDecl()->getLocStart(),
                                                                    *model.GetSourceManager());
                ErrorMessage(loc) << "in Agent " << agent.first << ", public attribute "
                                    << field.first << " is not of structural type (2)";

                model.AddErrorFound();
            }
        }
    }
}

void AreThereAPrivateAttributesOfNonStructuralType(Model &model, clang::ASTContext *context) {
    for (auto &agent : model.GetAgents()) {
        for (auto &field : agent.second.GetFields()) {
            if ((field.second.GetAccess() == clang::AS_private) &&
                (!IsStructuralType(field.second.GetType(), context))) {
                clang::FullSourceLoc loc = clang::FullSourceLoc(agent.second.GetDecl()->getLocStart(),
                                                                *model.GetSourceManager());
                WarningMessage(loc) << "in Agent " << agent.first << ", private attribute "
                                    << field.first << " is not of structural type. Setting class "
                                    << agent.first << " to unsendable";
				field.second.SetNotSendable();
				agent.second.SetNotSendable();
                model.AddWarningFound();
            }
        }
    }
}

void DoesAnAgentContainsAnAttributeDefinedAsStatic(Model &model) {

}

void IsAnAttributeOfAnAgentDefinedAsPrivateAndCritical(Model &model) {
    for (const auto &agent : model.GetAgents()) {
        for (const auto &field : agent.second.GetFields()) {
            if ((field.second.GetAccess() == clang::AS_private) && (field.second.IsCritical())) {
                clang::FullSourceLoc loc = clang::FullSourceLoc(agent.second.GetDecl()->getLocStart(),
																*model.GetSourceManager());
                ErrorMessage(loc) << "in Agent " << agent.first << ", private attribute "
                                    << field.first << " is also critical";

                model.AddErrorFound();
            }
        }
    }
}

void IsThereAnAnonymousStructInAttributes(Model &model) {
	for (const auto &agent : model.GetAgents())
		for (const auto &field : agent.second.GetFields())
			if (field.second.GetType().getAsString().substr(0,11) == "struct (ano") {
				clang::FullSourceLoc loc = clang::FullSourceLoc(agent.second.GetDecl()->getLocStart(),
																*model.GetSourceManager());
                ErrorMessage(loc) << "in Agent " << agent.first << ", type of attribute "
								  << field.first << " is an anonymous structure, which is forbidden.";
                model.AddErrorFound();
			}

	for (const auto &interaction : model.GetInteractions())
		for (const auto &field : interaction.second.GetFields())
			if (field.second.GetType().getAsString().substr(0,11) == "struct (ano") {
				clang::FullSourceLoc loc = clang::FullSourceLoc(interaction.second.GetDecl()->getLocStart(),
																*model.GetSourceManager());
                ErrorMessage(loc) << "in Agent " << interaction.first << ", type of attribute "
								  << field.first << " is an anonymous structure, which is forbidden.";
                model.AddErrorFound();
			}
}

void CheckErrorInModel(Model &model, clang::ASTContext *context) {
    IsThereMethodOrPrivateAttributesInInteraction(model);
    AreThereAPrivateAttributesOfNonStructuralType(model, context);
    ArePublicAttributsOfStruturalTypeInInteractionOrAgent(model, context);
    DoesAnAgentContainsAnAttributeDefinedAsStatic(model);
    IsAnAttributeOfAnAgentDefinedAsPrivateAndCritical(model);
	IsThereAnAnonymousStructInAttributes(model);
}
