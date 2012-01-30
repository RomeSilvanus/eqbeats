#include "../html.h"
#include "../../session.h"
#include "../../utils.h"
#include "../../account.h"
#include <sstream>

using namespace std;

string followButton(const User &u, int uid){
    if(uid == u.id()) return "";
    bool isFollowed = User(uid).isFollowing(u.id());
    return (string)"<a class=\""+(isFollowed?"unfollow":"follow")+"\" href=\"" + u.url() + "/" + 
                (isFollowed?"un":"") + "follow\">"+(isFollowed?"Stop following":"Follow")+"</a>";
}

string Html::userPage(int uid){
    Account user(uid);
    if(!user) return notFound("User");
    stringstream s;
    s << header(escape(user.name()), atomFeed(user.url() + "/atom"))
      << followButton(user, Session::user().id())
      << "<h2>" + escape(user.name()) + "</h2>"
      << "<div class=\"user\">"
             "<div class=\"email\"><img src=\"/static/mail.png\" /> Email: " << escapeEmail(user.email()) << "</div>";
    string about = user.about();
    if(!about.empty())
        s << "<div class=\"notes\">" << format(about) << "</div>";
    bool edition = Session::user().id() == user.id();
    if(edition)
        s << "<a class=\"more\" href=\"/account\">Edit</a><br /><br />";
    s << "<a class=\"more\" href=\"" << user.url() << "/favorites\">Favorite tracks</a>"
         "</div>"
         "<h3><img src=\"/static/disc.png\" /> Tracks " + feedIcon(user.url() + "/atom") + "</h3>"
      << Html::trackList(Track::byArtist(user.id(), edition), edition ? Html::Edition : Html::Compact);
    if(edition)
        s << uploadForm("/track/new") << "<h3><img src=\"/static/plus-circle.png\" /> Artists you follow</h3>" << Html::userList(user.following())
          << Html::comments(Comment::forArtist(uid), "Comments on your tracks");
    s << Html::comments(Comment::forUser(uid)) << Html::commentForm(user.url()+"/comment");
    s << footer();
    return s.str();
}

string Html::favorites(int uid){
    User u(uid);
    if(!u) return notFound("User");
    return Html::tracksPage(Html::escape(u.name())+" - Favorite tracks", Track::favorites(uid));
}

string Html::userList(const vector<User> &users){
    if(users.empty()) return "<div class=\"empty\">Nobody here yet.</div>";
    stringstream s(stringstream::out);
    s << "<ul>";
    for(vector<User>::const_iterator i=users.begin(); i!=users.end(); i++)
        s << "<li><a href=\"" << i->url() << "\">" << escape(i->name()) << "</a></li>";
    s << "</ul>";
    return s.str();
}

string Html::usersPage(){
    return header("Users")
         + "<h2>Users</h2>"
         + searchForm("/users/search")
         + userList(User::list(50))
         + footer();
}

string Html::artistsPage(){
    return header("Artists")
         + "<h2>Artists</h2>"
         + searchForm("/users/search")
         + userList(User::listArtists(200))
         + footer();
}

string Html::searchForm(const string &action, const string &q){
    return
        "<form action=\"" + action + "\">"
            "<input type=\"text\" name=\"q\" value=\"" + escape(q) + "\" />"
            "<input type=\"submit\" value=\"Search\" />"
        "</form>";
}

string Html::userSearch(const std::string &q){
    vector<User> res = User::search(q);
    stringstream s;
    s << header("User search")
      << "<h2>User search</h2>"
      << searchForm("/users/search", q);
    if(res.empty())
        s << "No result.";
    else
        s << userList(res);
    s << footer();
    return s.str();
}