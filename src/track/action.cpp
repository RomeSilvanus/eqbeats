#include <stdio.h>
#include <algorithm>
#include "actions.h"
#include "../art.h"
#include "../session.h"
#include <text/text.h>
#include "../follower.h"
#include "../path.h"
#include "../log.h"
#include "../track.h"
#include "../mail.h"
#include "../cgi.h"
#include "../media.h"
#include "../event.h"

using namespace std;

std::string filter(const std::string &str){
    std::string buf;
    for(std::string::const_iterator i = str.begin(); i!=str.end(); i++){
        if(*i != '\n' && *i != '\r') buf += *i;
    }
    return buf;
}

void Action::publishTrack(int tid){
    if(tid != number(cgi("tid")))
        return; // Http::redirect(Track::url(tid));
    User u = Session::user();
    Track t(tid);
    if(u == t.artist() && t && !t.visible() && u &&
        cgi.getEnvironment().getRequestMethod() == "POST"){
        t.setVisible(true);
        t.bump();
        Event::publish(t);
        // Mail
        std::vector<std::string> emails = Follower(u).followers();
        std::string maildata =
            "From: EqBeats notification <notify@eqbeats.org>\n"
            "Message-ID: notify-t" + number(t.id()) + "\n"
            "Subject: " + filter("EqBeats notification: " + u.name() + " - " + t.title()) + "\n"
            "Precedence: bulk\n\n" +
            u.name() + " just published a new track : " + t.title() + "\n"
            "Listen to it here : " + eqbeatsUrl() + t.url() + "\n\n"
            "You're receiving this email because you're following " + u.name() + " on EqBeats.\n"
            "If you don't want to receive these notifications anymore, go to " + eqbeatsUrl() + u.url() + " and click \"Stop following\".";
        for(std::vector<std::string>::const_iterator i = emails.begin(); i!=emails.end(); i++)
            sendMail(i->c_str(), maildata.c_str());
    }
    return; // Http::redirect(t.url());
}

void Action::updateNotes(int tid){
    User u = Session::user();
    ExtendedTrack t(tid);
    if(u==t.artist() && u &&
       cgi.getEnvironment().getRequestMethod() == "POST" )
        t.setNotes(cgi("notes"));
    return; // Http::redirect(t.url());
}

void Action::renameTrack(int tid){
    User u = Session::user();
    Track t(tid);
    if(u==t.artist() && u && !cgi("title").empty() &&
       cgi.getEnvironment().getRequestMethod() == "POST" ){
        log("Renaming track: " + t.title() + " -> " + cgi("title") + " (" + number(t.id()) + ")");
        t.setTitle(cgi("title"));
        Media(t).updateTags();
    }
    return; // Http::redirect(t.url());
}

void Action::deleteTrack(int tid){
    User u = Session::user();
    Track t(tid);
    if(u!=t.artist() || !u)
        ;//Http::redirect(t.url());
    else if(cgi.getEnvironment().getRequestMethod()!="POST" || cgi("confirm")!="Delete")
        ;//deletionForm(t);
    else{
        log("Deleting track: " + t.title() + " (" + number(t.id()) + ")");
        Art art(tid);
        if(art) art.remove();
        Media(t).unlink();
        t.remove();
        //Http::redirect(u.url());
    }
}

void Action::setFlags(int tid){
    User u = Session::user();
    ExtendedTrack t(tid);
    if(u==t.artist() && t && cgi.getEnvironment().getRequestMethod()=="POST")
        t.setAirable(cgi.queryCheckbox("airable"));
    //Http::redirect(t.url());
}

void Action::reportTrack(int tid){
    if(cgi.getEnvironment().getRequestMethod() != "POST") return; // Http::redirect(Track::url(tid));
    Track t(tid);
    if(!t) return; //Html::notFound("Track");
    std::string path = eqbeatsDir() + "/reports";
    std::ofstream f(path.c_str(), std::ios_base::app);
    f << t.artist().id() << " " << t.artist().name() << " - " << t.id() << " " << t.title() << std::endl;
    f.close();
    //Http::redirect(t.url());
}

void Action::setTags(int tid){
    ExtendedTrack t(tid);
    if(Session::user()=t.artist() && t && cgi.getEnvironment().getRequestMethod() == "POST")
        t.setTags(cgi("tags"));
    //Http::redirect(t.url());
}