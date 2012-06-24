#include "comment.h"
#include "escape.h"
#include "page.h"
#include "../render.h"
#include "../../comment.h"
#include "../../user.h"
#include "../../session.h"
#include "../../track.h"

using namespace std;
using namespace Render;

void Html::comments(const vector<Comment> &cmts, const std::string title){
    o << "<h3 id=\"comments\">" << icon("balloon") << " " << title << "</h3><div>";
    if(cmts.empty())
        o << "<div class=\"empty\">No comments yet.</div>";
    for(vector<Comment>::const_iterator i=cmts.begin(); i!=cmts.end(); i++){
            o << "<div class=\"comment\">" << Html::format(i->contents) << "<br />"
                 "<div class=\"by\">by ";
            if(i->authorId)
            o << "<a href=\""<< User::url(i->authorId) << "\">" << Html::escape(i->authorName()) << "</a>";
        else
            o << Html::escape(i->authorName());
        if(i->trackId)
            o << " on <a href=\"" << Track::url(i->trackId) << "\">" << Html::escape(i->trackTitle) << "</a>";
        o << "</div></div>";
    }
    o << "</div>";
}

void Html::commentForm(const string &action, bool quick){
    o << "<form class=\"postcomment\" action=\"" << action << "\" method=\"post\">" ;
    if(quick)
        o << (Session::user()?"":"<input type=\"text\" name=\"name\" placeholder=\"Name\"/>")
          << "<input class=\"honeypot\" type=\"text\" name=\"url\" placeholder=\"If you can see this, don't fill it in\"/>"
          << "<input type=\"text\" name=\"msg\" placeholder=\"Leave a note\"/>"
             "<input type=\"submit\" value=\"Post\" onclick=\"this.form.submit();this.disabled=true;return false;\" />";
    else{
        o << (Session::user() ? "" : "Name : <input type=\"text\" name=\"name\" /><br />")
          << "<input class=\"honeypot\" type=\"text\" name=\"url\" placeholder=\"If you can see this, don't fill it in\"/>"
             "<textarea name=\"msg\"></textarea><br />"
             "<input type=\"submit\" value=\"Post a comment\" onclick=\"this.form.submit();this.disabled=true;return false;\" />";
    }
    o << "</form>";
}