#include "MainInt.h"
#include "OnMapInt.h"

#include "ItemFabric.h"
#include "MapClass.h"
#include "sync_random.h"

#include "MagicStrings.h"

#include "Debug.h"

#include <zlib.h>

ItemFabric::ItemFabric()
{
    idTable_.resize(100);
    id_ = 1;
}

void ItemFabric::Sync()
{
    if (MAIN_TICK % HASH_OFTEN == 1)
    {
        hash_last_ = hash_all();

        //Debug::UnsyncDebug().Save();

        Message msg;

        msg.type = Net::SYSTEM_TYPE;
        msg.text = Net::HASH;
        msg.from = hash_last_;

        IMainItem::mobMaster->net_client->Send(msg);
    }
}

void ItemFabric::UpdateProcessingItems()
{   
    if (remove_from_process_.size())
    {
        for (auto it = remove_from_process_.begin(); it != remove_from_process_.end(); ++it)
        {   
            auto to_del = std::find(process_table_.begin(), process_table_.end(), *it);
            if (to_del != process_table_.end())
                process_table_.erase(to_del);
        }       
    }

    if (add_to_process_.size())
    {
        for (auto it = add_to_process_.begin(); it != add_to_process_.end(); ++it)
        {
            auto to_add = std::find(process_table_.begin(), process_table_.end(), *it);
            if (to_add == process_table_.end())
                process_table_.push_back(*it);
        }
    }
    if (remove_from_process_.size() || add_to_process_.size())
    {
        std::sort(process_table_.begin(), process_table_.end(),
        [](id_ptr_on<IMainItem> item1, id_ptr_on<IMainItem> item2)
        {
            return item1.ret_id() < item2.ret_id();
        });
        remove_from_process_.clear();
        add_to_process_.clear();

        SYSTEM_STREAM << "Size of processed: " << process_table_.size() << std::endl;
        //for (auto it = process_table_.begin(); it != process_table_.end(); ++it)
        //    SYSTEM_STREAM << "Id: " << it->ret_id() << std::endl;
    }
}

void ItemFabric::foreachProcess()
{
    UpdateProcessingItems();

    size_t table_size = process_table_.size();
    for (size_t i = 0; i < table_size; ++i)
        if (process_table_[i].valid() && process_table_[i]->GetFreq() && ((MAIN_TICK % process_table_[i]->GetFreq()) == 0))
            process_table_[i]->process();

   /* size_t table_size = idTable_.size();
    for (size_t i = 1; i < table_size; ++i)
        if (idTable_[i] != nullptr && idTable_[i]->GetFreq() && ((MAIN_TICK % idTable_[i]->GetFreq()) == 0))
            idTable_[i]->process();*/

    UpdateProcessingItems();
}

void ItemFabric::saveMapHeader(std::stringstream& savefile)
{
    savefile << MAIN_TICK << std::endl;
    savefile << id_ << std::endl;
    savefile << IMainItem::map->mobi->thisMob.ret_id() << std::endl;

    // Random save
    savefile << random_helpers::get_seed() << std::endl;
    savefile << random_helpers::get_calls_counter() << std::endl;

    savefile << IMainItem::mobMaster->GetCreator() << std::endl;
}

void ItemFabric::loadMapHeader(std::stringstream& savefile, size_t real_this_mob)
{
    savefile >> MAIN_TICK;
    SYSTEM_STREAM << "MAIN_TICK: " << MAIN_TICK << std::endl;

    savefile >> id_;
    SYSTEM_STREAM << "id_: " << id_ << std::endl;
    
    size_t loc;
    savefile >> loc;
    SYSTEM_STREAM << "thisMob: " << loc << std::endl;

    if (real_this_mob == 0)
        IMainItem::map->mobi->thisMob = loc;
    else
        IMainItem::map->mobi->thisMob = real_this_mob;
    
    unsigned int new_seed;
    unsigned int new_calls_counter;
    savefile >> new_seed;
    savefile >> new_calls_counter;

    random_helpers::set_rand(new_seed, new_calls_counter);

    size_t new_creator;
    savefile >> new_creator;
    SYSTEM_STREAM << "new_creator: " << new_creator << std::endl;

    IMainItem::mobMaster->SetCreator(new_creator);

    idTable_.resize(id_ + 1);
}

void ItemFabric::saveMap(const char* path)
{
    std::fstream rfile;
    rfile.open(path, std::ios_base::out | std::ios_base::trunc);
    if(rfile.fail()) 
    {
        SYSTEM_STREAM << "Error open " << path << std::endl; 
        return;
    }
    std::stringstream savefile;
    saveMap(savefile, false);
    rfile << savefile.str();
    rfile.close();
}
void ItemFabric::saveMap(std::stringstream& savefile, bool zip)
{
    saveMapHeader(savefile);
    auto it = ++idTable_.begin();
    while(it != idTable_.end())
        if(*it) 
        {
            (*it++)->saveSelf(savefile);
            savefile << std::endl;
        }
        else 
            ++it;
    savefile << "0 ~";

    if (zip)
    {
        SYSTEM_STREAM << "Begin zip save" << std::endl;

        std::string str = savefile.str();
        const unsigned char* raw_uncompressed = reinterpret_cast<const unsigned char*>(str.c_str());

        unsigned long len_compressed = static_cast<unsigned long>(str.length() * 1.1 + 20);
        unsigned char* raw_compressed = new unsigned char[len_compressed];

        int result = compress(raw_compressed, &len_compressed, raw_uncompressed, str.length());
        switch(result)
        {
        case Z_OK:
            SYSTEM_STREAM << "Zip is going ok" << std::endl;
            break;
        case Z_MEM_ERROR:
            SYSTEM_STREAM << "ERROR: Out of memory" << std::endl;
            break;
        case Z_BUF_ERROR:
            SYSTEM_STREAM << "ERROR: Buffer too small for data" << std::endl;
            break;
        }

        savefile.str("");
    
        SYSTEM_STREAM << "Begin load zipped to stream" << std::endl;

        for (int i = 0; i < len_compressed; ++i)
            savefile << raw_compressed[i];

        delete[] raw_compressed;
    }
}

void ItemFabric::loadMap(const char* path)
{
    std::stringstream savefile;
    std::fstream rfile;
    rfile.open(path, std::ios_base::in);

    rfile.seekg (0, std::ios::end);
    std::streamoff length = rfile.tellg();
    rfile.seekg (0, std::ios::beg);
    char* buff = new char[(unsigned int)(length + 1)];
    buff[length + 1] = '\0';

    rfile.read(buff, length);
    rfile.close();
    savefile << buff;
    delete[] buff;
    //
    loadMap(savefile, false);
}

const int AVERAGE_BYTE_PER_TILE = 129 * 2;
const long int UNCOMPRESS_LEN_DEFAULT = sizeHmap * sizeWmap * sizeDmap * AVERAGE_BYTE_PER_TILE;
void ItemFabric::loadMap(std::stringstream& savefile, bool zip, size_t real_this_mob)
{

    clearMap();

    if (zip)
    {
        std::string str = savefile.str();
        const unsigned char* raw_compressed = reinterpret_cast<const unsigned char*>(str.c_str());
        unsigned long len_compressed = static_cast<unsigned long>(str.length());

        unsigned long len_uncompressed = UNCOMPRESS_LEN_DEFAULT;
        unsigned long len_uncompressed_to_use;
    
        unsigned char* raw_uncompressed;
        SYSTEM_STREAM << "Begin cycle unzip map" << std::endl;
        while (true)
        {
            len_uncompressed_to_use = len_uncompressed;
            raw_uncompressed = new unsigned char[len_uncompressed_to_use];
            int result = uncompress(raw_uncompressed, &len_uncompressed_to_use, raw_compressed, len_compressed);

            if (result == Z_BUF_ERROR)
            {
                delete[] raw_uncompressed;
                len_uncompressed *= 2;
                continue;
            } 
            else if (result == Z_MEM_ERROR)
            {
                SYSTEM_STREAM << "Insufficient memory" << std::endl;
            } 
            else if (result == Z_DATA_ERROR)
            {
                SYSTEM_STREAM << "The compressed data (referenced by source) was corrupted" << std::endl;
            }
            else if (result == Z_OK)
            {
                SYSTEM_STREAM << "Unzip ok" << std::endl;
                break;
            }
        }

        savefile.str("");

        for (int i = 0; i < len_uncompressed_to_use; ++i)
            savefile << static_cast<char>(raw_uncompressed[i]);

        delete[] raw_uncompressed;
    }

    auto str_t = savefile.str();
    

    std::cout << std::endl << str_t.substr(str_t.size() - 500) << std::endl;

    loadMapHeader(savefile, real_this_mob);
    int j = 0;
    while(!savefile.eof())
    {
        j++;
        if(savefile.fail())
        {
            SYSTEM_STREAM << "Error! " << j << "\n";
            SDL_Delay(10000);
        }
        unsigned int type;
        savefile >> type;
        if(type == 0)
        {
            SYSTEM_STREAM << "Zero id reached" << std::endl;
            break;
        }

        //SYSTEM_STREAM << "Line number: " << j << std::endl;

        size_t id_loc;
        savefile >> id_loc;
 
        size_t id_master;
        savefile >> id_master;
        
        id_ptr_on<IMainItem> i;
        i = newItemSaved<IMainItem>(id_master, type, id_loc);
        i->loadSelf(savefile);
    }
    SYSTEM_STREAM << "\n NUM OF ELEMENTS CREATED: " << j << "\n";
    IMainItem::map->mobi->changeMob(IMainItem::map->mobi->thisMob);
}

IMainItem* ItemFabric::newVoidItem(unsigned int type)
{
    static Initer init;
    return (*itemList())[type]();
};

IMainItem* ItemFabric::newVoidItemSaved(unsigned int type)
{
    static Initer init;
    return (*itemListSaved())[type]();
};

void ItemFabric::clearMap()
{
    size_t table_size = idTable_.size();
    for (size_t i = 1; i < table_size; ++i)
        if (idTable_[i] != nullptr)
            idTable_[i]->delThis();
    if (table_size != idTable_.size())
        SYSTEM_STREAM << "WARNING: table_size != idTable_.size()!" << std::endl;

    process_table_.clear();
    add_to_process_.clear();
    remove_from_process_.clear();
};

unsigned int ItemFabric::hash_all()
{
    unsigned int h = 0;
    size_t table_size = idTable_.size();
    for (size_t i = 1; i < table_size; ++i)
        if (idTable_[i] != nullptr)
            h += idTable_[i]->hashSelf();
    return h;
}



void ItemFabric::AddProcessingItem(id_ptr_on<IMainItem> item)
{
    add_to_process_.push_back(item);
}

void ItemFabric::RemoveProcessingItem(id_ptr_on<IMainItem> item)
{
    remove_from_process_.push_back(item);  
}