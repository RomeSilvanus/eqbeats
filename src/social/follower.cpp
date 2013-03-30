#include "follower.h"
#include <core/db.h>
#include <text/text.h>

AccountList Follower::followed(){
    return AccountList("SELECT %s FROM users, favorites "
        "WHERE favorites.ref = users.id AND favorites.type = 'artist' "
        "AND favorites.user_id = " + number(id) + " "
        "ORDER BY users.name ASC");
}

AccountList Follower::followers(){
    return AccountList("SELECT %s FROM users, favorites "
        "WHERE favorites.ref = " + number(id) + " AND favorites.type = 'artist' "
        "AND favorites.user_id = users.id "
        "ORDER BY users.name ASC");
}

bool favHelper(const std::string &t, int ref, int id){
    DB::Result r = DB::query(
        "SELECT EXISTS (SELECT 1 FROM favorites "
            "WHERE favorites.type = '" + t + "' "
            "AND favorites.user_id = " + number(id) + " "
            "AND favorites.ref = " + number(ref) +
        ")");
    if(r.empty()) return false;
    return r[0][0] == "t";
}

bool Follower::following(int uid){
    return favHelper("artist", uid, id);
}

bool Follower::favorited(int tid){
    return favHelper("track", tid, id);
}

TrackList Follower::favorites(){
    return TrackList(
        "SELECT %s FROM %s, favorites WHERE %s AND favorites.type = 'track' "
        "AND favorites.ref = tracks.id AND favorites.user_id = " + number(id) + " "
        "ORDER BY users.name ASC");
}
