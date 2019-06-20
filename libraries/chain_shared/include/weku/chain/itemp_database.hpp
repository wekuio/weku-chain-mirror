#pragma once

#include <fc/uint128.hpp>
#include <chainbase/chainbase.hpp>

#include <weku/protocol/block.hpp>
#include <weku/protocol/weku_operations.hpp>
#include <weku/protocol/config.hpp>

#include <weku/chain/util/asset.hpp>
#include <weku/chain/util/reward.hpp>
#include <weku/chain/util/uint256.hpp>
#include <weku/chain/weku_object_types.hpp>
#include <weku/chain/account_objects.hpp>
#include <weku/chain/global_property_object.hpp>
#include <weku/chain/hardfork_property_object.hpp>
#include <weku/chain/common_objects.hpp>
#include <weku/chain/comment_objects.hpp>
#include <weku/chain/node_property_object.hpp>
#include <weku/chain/block_summary_object.hpp>
#include <weku/chain/database_exceptions.hpp>
#include <weku/chain/fork_database.hpp>
#include <weku/chain/transaction_object.hpp>
#include <weku/chain/witness_objects.hpp>
#include <weku/chain/compound.hpp>
#include <weku/chain/i_hardforker.hpp>
#include <weku/chain/block_log.hpp>
#include <weku/chain/fork_database.hpp>
#include <weku/chain/hardfork_constants.hpp>

using namespace weku::protocol;

namespace weku{ namespace chain{

enum validation_steps
    {
    skip_nothing                = 0,
    skip_witness_signature      = 1 << 0,  ///< used while reindexing
    skip_transaction_signatures = 1 << 1,  ///< used by non-witness nodes
    skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
    skip_fork_db                = 1 << 3,  ///< used while reindexing
    skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
    skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
    skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
    skip_merkle_check           = 1 << 7,  ///< used while reindexing
    skip_undo_history_check     = 1 << 8,  ///< used while reindexing
    skip_witness_schedule_check = 1 << 9,  ///< used while reindexing
    skip_validate               = 1 << 10, ///< used prior to checkpoint, skips validate() call on transaction
    skip_validate_invariants    = 1 << 11, ///< used to skip database invariant check on block application
    skip_undo_block             = 1 << 12, ///< used to skip undo db on reindex
    skip_block_log              = 1 << 13  ///< used to skip block logging on reindex
};

// this class is to tempararily used to refactory database
class itemp_database: public chainbase::database 
{
    public:
    virtual ~itemp_database() = default;
    virtual void init_genesis(uint64_t initial_supply);

    virtual uint32_t head_block_num()const;
    virtual uint32_t last_hardfork();
    virtual void last_hardfork(uint32_t hardfork);  
    virtual bool has_hardfork( uint32_t hardfork )const;

    virtual hardfork_votes_type next_hardfork_votes();
    virtual void next_hardfork_votes(hardfork_votes_type next_hardfork_votes);

    virtual const dynamic_global_property_object&  get_dynamic_global_properties()const;
    virtual const witness_schedule_object&         get_witness_schedule_object()const;
    virtual const feed_history_object&             get_feed_history()const;

    virtual block_id_type head_block_id()const;
    virtual fc::time_point_sec head_block_time()const;

    virtual const account_object&  get_account(  const account_name_type& name )const;
    virtual const account_object*  find_account( const account_name_type& name )const;
    virtual const witness_object&  get_witness(  const account_name_type& name )const;
    virtual const witness_object*  find_witness( const account_name_type& name )const;

    virtual void apply_block( const signed_block& next_block, uint32_t skip = skip_nothing );

    virtual void adjust_savings_balance( const account_object& a, const asset& delta );
    virtual void adjust_reward_balance( const account_object& a, const asset& delta );
    virtual void adjust_balance( const account_object& a, const asset& delta );
    virtual void adjust_supply( const asset& delta, bool adjust_vesting = false );
    virtual void adjust_witness_votes( const account_object& a, share_type delta );
    virtual void adjust_witness_vote( const witness_object& obj, share_type delta );
    virtual void adjust_rshares2( const comment_object& comment, fc::uint128_t old_rshares2, fc::uint128_t new_rshares2 );
    virtual void adjust_proxied_witness_votes( const account_object& a,
                                            const std::array< share_type, STEEMIT_MAX_PROXY_RECURSION_DEPTH+1 >& delta,
                                            int depth = 0 );
    virtual void adjust_liquidity_reward( const account_object& owner, const asset& volume, bool is_bid );
    virtual const void push_virtual_operation( const operation& op, bool force = false ); 
    
    virtual void cancel_order( const limit_order_object& obj );

    virtual const reward_fund_object& get_reward_fund( const comment_object& c )const;
    virtual uint16_t get_curation_rewards_percent( const comment_object& c ) const;
    virtual share_type pay_curators( const comment_object& c, share_type& max_rewards );
    virtual asset create_vesting( const account_object& to_account, asset steem, bool to_reward_balance=false );
    
    virtual void adjust_total_payout( const comment_object& a, const asset& sbd, const asset& curator_sbd_value, const asset& beneficiary_value );
    virtual void validate_invariants()const;
    virtual const fc::time_point_sec calculate_discussion_payout_time( const comment_object& comment )const;
    virtual asset to_sbd( const asset& steem )const;
    virtual share_type pay_reward_funds( share_type reward );

    virtual asset get_content_reward()const ;
    virtual asset get_producer_reward() ;
    virtual asset get_curation_reward()const ;
    virtual asset get_pow_reward()const ;
    virtual account_name_type get_scheduled_witness(uint32_t slot_num)const;
    virtual uint32_t get_slot_at_time(fc::time_point_sec when)const;
    virtual const node_property_object& get_node_properties()const;
    virtual node_property_object& node_properties();

    virtual block_log& get_block_log(); 
    virtual fork_database& fork_db();

    virtual void adjust_proxied_witness_votes( const account_object& a, share_type delta, int depth = 0 );
    virtual void clear_witness_votes( const account_object& a );
};

}}