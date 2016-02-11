#include <Quest.h>

#include <entities.h>

extern Player *player;

int QuestHandler::assign(std::string title,std::string desc,std::string req){
	Quest tmp;
	char *tok;
	
	tmp.title = title;
	tmp.desc = desc;

	std::unique_ptr<char[]> buf (new char[req.size()]);

	strcpy(buf.get(),req.c_str());
	tok = strtok(buf.get(),"\n\r\t,");
	tmp.need.push_back({"\0",0});
	
	while(tok){
		if(tmp.need.back().name != "\0"){
			tmp.need.back().n = atoi(tok);
			tmp.need.push_back({"\0",0});
		}else
			tmp.need.back().name = tok;
		
		tok = strtok(NULL,"\n\r\t,");
	}
	
	tmp.need.pop_back();
	current.push_back(tmp);

	return 0;
}

int QuestHandler::drop(std::string title){
	for(unsigned int i=0;i<current.size();i++){
		if(current[i].title == title){
			current.erase(current.begin()+i);
			return 0;
		}
	}
	return -1;
}

int QuestHandler::finish(std::string t){
	for(unsigned int i=0;i<current.size();i++){
		if(current[i].title == t){
			for(auto &n : current[i].need){
				if(player->inv->hasItem(n.name) < n.n)
					return 0;
			}
			
			for(auto &n : current[i].need){
				player->inv->takeItem(n.name,n.n);
			}
			
			current.erase(current.begin()+i);
			return 1;
		}
	}
	return 0;
}

bool QuestHandler::hasQuest(std::string t){
	for(unsigned int i=0;i<current.size();i++){
		if(current[i].title == t)
			return true;
	}
	return false;
}
