/**
 * \file interaction.hpp
 * \brief Defines the particular type of messages between masters which
 *        represents messages between agents.
 */

#ifndef INTERACTION_HPP_
#define INTERACTION_HPP_

#include <vector>
#include <memory>
#include <mpi.h>

#include "types.hpp"


/**
 * \struct InteractionStruct
 * \brief Prototype of the structures used to send interactions between masters
 *        using MPI, containing the first fields that are needed.
 *
 * \details The recipient and sender agent ids and types must be explicitly
 * written, as well as the interaction type, and they will be followed in each
 * specific interaction structure by a structure containing the set of
 * attributes of the interactions.
 *
 * In order to be able to create their MPI_Datatype, this structures generated
 * in the precompilation step won't actually inherit InteractionStruct, but will
 * have the same first fields.
 */
struct InteractionStruct {
	/// The first attribute this type of structure has to store is the interaction type.
	InteractionType type;

	/// Local identifier of the agent which sent the message.
	AgentId sender_id;

	/// Type identifier of the agent which sent the message.
	AgentType sender_type;

	/// Local identifier of the agent to which the message is sent.
	AgentId recipient_id;

	/// Type identifier of the agent to which the message is sent.
	AgentType recipient_type;
};


/**
 * \class Interaction

 * \brief Interaction is the type of master-master messages which represents the
 *        interactions (message sending) between agents.
 *
 * \details An interaction must contain data about the agent which sent it and
 * about the agent which will receive it. These indentifiers must be able to be
 * read by masters which will manipulate them.
 *
 * Sending interactions represented by classes with MPI is difficult, therefore
 * an interaction always contains its representation that is of the form of a
 * data structure inheriting InteractionStruct; class Interaction provides
 * a method FromStruct which generates the interaction corresponding to the data
 * structure given as input.
 */
class Interaction {
	friend class Master;

public:
	Interaction (const Interaction &e) {
		structure_ = nullptr;
	}
	void operator= (const Interaction &e) {
		structure_ = nullptr;
	}

	/**
	 * \fn Interaction (InteractionType type, AgentId sender_id, AgentType sender_type,
     *                  AgentId recipient_id, AgentType recipient_type)
	 * \brief Constructor of Interaction.
	 * \param type Type identifier of the interaction.
	 * \param sender_id Local identifier of the agent which sends this
	 *        interaction.
	 * \param sender_type Type identifier of the agent which sends this
	 *        interaction.
	 * \param recipient_id Local identifier of the agent which this interaction
	 *        is destined to.
	 * \param recipient_type Type identifier of the agent which this interaction
	 *        is destined to.
	 */
	Interaction (InteractionType type, AgentId sender_id, AgentType sender_type,
		AgentId recipient_id, AgentType recipient_type) :

		type_{type}, sender_id_{sender_id}, sender_type_{sender_type},
		recipient_id_{recipient_id}, recipient_type_{recipient_type}
	{
	};

	/**
	 * \fn ~Agent()
	 * \brief Destructor of an agent.
	 * \details Frees the field structure_ if needed.
	 */
	~Interaction() {
		if (structure_ != nullptr) {
			free(structure_);
		}
	}

	/// Getter of type_.
	InteractionType GetType() const {
		return type_;
	}

	/// Getter of sender_id_.
	AgentId GetSenderId() const {
		return sender_id_;
	}

	/// Getter of sender_type_.
	AgentType GetSenderType() const {
		return sender_type_;
	}

	/// Getter of recipient_id_.
	AgentId GetRecipientId() const {
		return recipient_id_;
	}

	/// Getter recipient_type_.
	AgentType GetRecipientType() const {
		return recipient_type_;
	}

	/// Getter of structure_
	void* GetStructure() {
		return structure_;
	}

	/// Setter of structure_
	void SetStructure(void* new_structure) {
		structure_ = new_structure;
	}

	/**
	 * \fn static std::unique_ptr<Interaction> FromStruct(void *s)
	 * \brief Builds and returns the interaction represented by the structure
	 *        given as input.
	 * \param s Pointer to a structure representing an interaction.
	 * \return The unique_ptr pointing towards the interaction represented by s.
	 * \details Takes as input the structure representing an interaction and
	 * giving the type of the interaction, creates the corresponding interaction
	 * and returns it in a unique_ptr.
	 * \remark Generated in the precompilation step.
	 */
	static std::unique_ptr<Interaction> FromStruct(void *s);

protected:

	/// Type identifier of the interaction
	InteractionType type_;

	/// Local identifier of the agent which sent the message.
	AgentId sender_id_;

	/// Type identifier of the agent which sent the message.
	AgentType sender_type_;

	/// Local identifier of the agent to which the message is sent.
	AgentId recipient_id_;

	/// Type identifier of the agent to which the message is sent.
	AgentType recipient_type_;

	/**
	 * Pointer to the data structure representing the interaction class (used to
	 * send it with MPI), which inherits InteractionStruct.
	 */
	void* structure_;

};

#endif
