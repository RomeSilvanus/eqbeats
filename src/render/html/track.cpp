#include "track.h"
#include "page.h"
#include "escape.h"
#include "player.h"
#include "comment.h"
#include "forms.h"
#include "../http.h"
#include "../render.h"
#include "../../art.h"
#include "../../comment.h"
#include "../../track.h"
#include "../../session.h"
#include "../../number.h"
#include "../../path.h"
#include "../../follower.h"
#include <algorithm>

using namespace std;
using namespace Render;

void Html::trackPage(int tid){
    ExtendedTrack t(tid);
    if(!t) return notFound("Track");
    bool edition = Session::user().id() == t.artist().id();
    Art art(tid);
    unsigned hits = edition ? t.getHits() : t.hit();
    string path = getPath();

    // Header
    header(t.title(), (t.notes().empty()?"":metaDescription(strip(t.notes()))) +
            "<link rel=\"alternate\" type=\"application/json+oembed\" href=\"" + eqbeatsUrl() + "/oembed?format=json&amp;url=http%3A//eqbeats.org" + path + "\" />"
            "<link rel=\"alternate\" type=\"text/xml+oembed\" href=\"" + eqbeatsUrl() + "/oembed?format=xml&amp;url=http%3A//eqbeats.org" + path + "\" />");

    // Title
    o << "<div class=\"track\">"
      << "<h2>" << escape(t.title()) << " ";
    bool isFav = Follower(Session::user()).isFavorite(t.id());
    o << "<a href=\"" << (Session::user() ? "" : "/login?redirect=") << t.url() << "/" << (isFav?"un":"") << "favorite\""
         " title=\"" << (isFav?"Remove from favorites":"Add to favorites") << "\">"
           "<img src=\"/static/star" << (isFav?"":"-empty") << ".png\""
             " alt=\"" << (isFav?"Remove from favorites":"Add to favorites") << "\"/></a>"
         "</h2>"
         "<h4>by <a href=\"" << t.artist().url() <<  "\">" << escape(t.artist().name()) << "</a></h4>"

      << (art?"<img class=\"art\" alt=\"\" src=\"" + art.url(Art::Medium) + "\" />":"");

    player(t);

    // Toolbar
    o << "<div class=\"toolbar\">";
    if(t.downloadable()||art){
        o << "<span><img src=\"/static/drive-download.png\" alt=\"\" /> Download : ";
        if(t.downloadable())
            o << "<a href=\"" << t.url(Track::Vorbis) << "\">OGG Vorbis</a> "
                 "<a href=\"" << t.url(Track::MP3) << "\">MP3</a> ";
        if (art) o << "<a href=\"" + art.url() + "\" target=\"_blank\">Art</a>";
        o << "</span> ";
    }
    o << "<span><img src=\"/static/balloon-white-left.png\" alt=\"\" /> Share : <a href=\"#embedcode\" onclick=\"document.getElementById('embedcode').style.display='block';return false;\">Embed</a></span>";
    if(edition){
        if(hits)
            o << " <span><img src=\"/static/edit-number.png\" alt=\"\" /> Hits : " << hits << "</span>";
        o << " <span><img src=\"/static/star.png\" alt=\"\" /> Favourites : " << t.favoritesCount() << "</span>";
    }
    else
        o << " <form action=\"" << t.url() << "/report\" method=\"post\">"
                "<button type=\"submit\" class=\"report\"><img src=\"/static/flag.png\" alt=\"\" /><span>Report</span></button>"
             "</form>";
    o << "</div>"
    // Embed
         "<textarea id=\"embedcode\" style=\"display:none;\">" << Html::embedTrackCode(t) << "</textarea>";

    // Tags
    if(!t.tags().empty() || edition){
        o << "<div class=\"toolbar tags\"><img src=\"/static/tag.png\" alt=\"\" /> Tags:";
        vector<string> ts = t.tags();
        if(edition){
            o << " <form action=\"" << t.url() << "/tags\" method=\"post\">"
                    "<input name=\"tags\" value=\"";
            for(vector<string>::const_iterator i=ts.begin(); i!=ts.end(); i++){
                if(i != ts.begin()) o << ", ";
                o << escape(*i);
            }
            o <<    "\" /> <input type=\"submit\" value=\"Update\" /> <span class=\"legend\">(comma-separated, e.g. instrumental, electronic)</span></form>";
        }
        else{
            for(vector<string>::const_iterator i=ts.begin(); i!=ts.end(); i++)
                o << " <a href=\"/tracks/tag/" << escape(*i) << "\">" << escape(*i) << "</a>";
        }
        o << "</div>";
    }

    // Notes
    if(!t.notes().empty())
        o << "<div class=\"notes\">" << format(t.notes()) << "</div>";

    if(edition){
        o << "<div class=\"edit\">"
             "<h3><img src=\"/static/pencil.png\" alt=\"\" /> Edit</h3>";
        // Publishing
        if(!t.visible())
            o << "<form class=\"publish\" action=\"" << t.url() << "/publish\" method=\"post\">"
                     "<img src=\"/static/disc-arrow.png\" alt=\"\" />"
                     " This track is not yet published."
                     " <input type=\"submit\" value=\"Publish\"/>"
                     "<input type=\"hidden\" name=\"tid\" value=\"" << number(t.id()) << "\"/>"
                 "</form>";
        // Rename
        o << "<div class=\"rename\">"
             "<h4><img src=\"/static/rename.png\" alt=\"\" /> Rename</h4>"
             "<form method=\"post\" action=\"" << t.url() << "/rename\">"
                 "<b>" << Html::escape(t.artist().name()) << "</b> - "
                 "<input type=\"text\" name=\"title\" value=\"" << Html::escape(t.title()) << "\" />"
                 "<input type=\"submit\" value=\"Rename\" />"
             "</form>"
             "</div>"
        // Reupload
             "<div class=\"column\">"
             "<h4><img src=\"/static/drive-upload.png\" alt=\"\" /> Re-upload</h4>";
        uploadForm(t.url()+"/upload");
        // Art
        o << "<h4><img src=\"/static/picture.png\" alt=\"\" /> Art</h4>"
             "<form action=\"" << t.url() << "/art/upload\" method=\"post\" enctype=\"multipart/form-data\">"
                 "<input type=\"file\" name=\"file\" />"
                 "<input type=\"submit\" value=\"Upload a picture\" />"
             "</form>"
        // Flags
             "<h4><img src=\"/static/flag-yellow.png\" alt=\"\" /> Flags</h4>"
             "<form action=\"" << t.url() << "/flags\" method=\"post\">"
                 "<input type=\"checkbox\" name=\"downloadable\" "
                   << (t.downloadable() ? "checked=\"checked\" " : "") << " /> "
                 "Downloadable<br />"
                 "<input type=\"checkbox\" name=\"airable\" "
                   << (t.airable() ? "checked=\"checked\" " : "") << " /> "
                 "Airable <span class=\"legend\">(on Celestia Radio)</span><br />"
                 "<input type=\"submit\" value=\"Update\" />"
             "</form>"
             "</div>"
             "<div class=\"column\">"
        // Notes
             "<h4><img src=\"/static/card-pencil.png\" alt=\"\" /> Notes</h4>"
             "<form action=\"" << t.url() << "/notes\" method=\"post\">"
                 "<textarea name=\"notes\">" << escape(t.notes()) << "</textarea><br />"
                 "<div class=\"legend\">(tags: [b]old, [u]nderline, [i]talic)</div>"
                 "<input type=\"submit\" value=\"Update description\" />"
             "</form>"
        // End
             "</div>"
             "<a class=\"delete\" href=\"" << t.url() << "/delete\">Delete track</a>"
             "<div style=\"clear:both;\"></div>"
             "</div>";
    }

    // Comments
    comments(Comment::forTrack(t.id()));
    commentForm(t.url() + "/comment");

    o << "</div>";
    footer();
}
