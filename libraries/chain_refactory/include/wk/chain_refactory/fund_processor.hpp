#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class fund_processor{
    public:
        fund_processor(itemp_database& db):_db(db){}
        void process_funds();
    private:
        itemp_database& _db;
};

}}