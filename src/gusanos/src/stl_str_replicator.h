#ifndef STL_STR_REPLICATOR_H
#define STL_STR_REPLICATOR_H

#include "netstream.h"
#include <string>
#include <stdexcept>

class STLStringReplicator : public Net_ReplicatorBasic
{
	private:
	
		std::string*	m_ptr;
		std::string	m_cmp;
	
	public:

		STLStringReplicator(Net_ReplicatorSetup *_setup, std::string *_data);
	
	// TODO: Implement this for safeness sake
		Net_Replicator* Duplicate(Net_Replicator *_dest)
		{
			if(_dest)
				*_dest = *this;
			else
				return new STLStringReplicator(*this);
			return 0;
		}
	
		bool checkState();
	
		bool checkInitialState() { return true; }
	
		void packData(Net_BitStream *_stream);
	
		void unpackData(Net_BitStream *_stream, bool _store, Net_U32 _estimated_time_sent);
	
		void Process(eNet_NodeRole _localrole, Net_U32 _simulation_time_passed) {};
	
		void* peekData();
	
		void clearPeekData();
};

#endif
