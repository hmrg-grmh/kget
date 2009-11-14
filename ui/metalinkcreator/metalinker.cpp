/***************************************************************************
*   Copyright (C) 2009 Matthias Fuchs <mat69@gmx.net>                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
***************************************************************************/

#include "metalinker.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtXml/QDomElement>

#include <kdeversion.h>
#include <KDebug>
#include <KLocale>
#include <KSystemTimeZone>

#ifdef HAVE_NEPOMUK
#include <Nepomuk/Variant>
#endif //HAVE_NEPOMUK

const QStringList KGetMetalink::DateConstruct::WEEKDAYS = (QStringList() << "" << "Mon" << "Tue" << "Wed" << "Thu" << "Fri" << "Sat" << "Sun");
const QStringList KGetMetalink::DateConstruct::MONTHS = (QStringList() << "" << "Jan" << "Feb" << "Mar" << "Apr" << "May" << "Jun" << "Jul" << "Aug" << "Sep" << "Oct" << "Nov" << "Dec");
const QString KGetMetalink::Metalink::KGET_DESCRIPTION = QString(QString("KGet ") + "2." + QString::number(KDE_VERSION_MINOR) + '.' + QString::number(KDE_VERSION_RELEASE));

namespace KGetMetalink
{
    QString addaptHashType(const QString &type, bool loaded);
}

/**
 * Adapts type to the way the hash is internally stored
 * @param type the hash-type
 * @param load true if the hash has been loaded, false if it should be saved
 * @note metalink wants sha1 in the form "sha-1", though
 * the metalinker uses it internally in the form "sha1", this function
 * transforms it to the correct form, it is only needed internally
*/
QString KGetMetalink::addaptHashType(const QString &type, bool loaded)
{
    QString t = type;
    if (loaded)
    {
        t.replace("sha-", "sha");
    }
    else
    {
        t.replace("sha", "sha-");
    }

    return t;
}

void KGetMetalink::DateConstruct::setData(const QDateTime &dateT, const QTime &timeZoneOff, bool negOff)
{
    dateTime = dateT;
    timeZoneOffset = timeZoneOff;
    negativeOffset = negOff;
}

void KGetMetalink::DateConstruct::setData(const QString &dateConstruct)
{
    if (dateConstruct.isEmpty())
    {
        return;
    }

    const QString exp = "yyyy-MM-ddThh:mm:ss";
    const int length = exp.length();

    dateTime = QDateTime::fromString(dateConstruct.left(length), exp);
    if (dateTime.isValid())
    {
        int index = dateConstruct.indexOf('+', length - 1);
        if (index > -1)
        {
            timeZoneOffset = QTime::fromString(dateConstruct.mid(index + 1), "hh:mm");
        }
        else
        {
            index = dateConstruct.indexOf('-', length - 1);
            if (index > -1)
            {
                negativeOffset = true;
                timeZoneOffset = QTime::fromString(dateConstruct.mid(index + 1), "hh:mm");
            }
        }
    }
}

bool KGetMetalink::DateConstruct::isNull() const
{
    return dateTime.isNull();
}

bool KGetMetalink::DateConstruct::isValid() const
{
    return dateTime.isValid();
}

QString KGetMetalink::DateConstruct::toString() const
{
    QString string;

    if (dateTime.isValid())
    {
        string += dateTime.toString(Qt::ISODate);
    }

    if (timeZoneOffset.isValid())
    {
        string += (negativeOffset ? '-' : '+');
        string += timeZoneOffset.toString("hh:mm");
    }
    else if (!string.isEmpty())
    {
        string += 'Z';
    }

    return string;
}

void KGetMetalink::DateConstruct::clear()
{
    dateTime = QDateTime();
    timeZoneOffset = QTime();
}

void KGetMetalink::UrlText::clear()
{
    name.clear();
    url.clear();
}

void KGetMetalink::CommonData::load(const QDomElement &e)
{
    identity = e.firstChildElement("identity").text();
    version = e.firstChildElement("version").text();
    description = e.firstChildElement("description").text();
    logo = KUrl(e.firstChildElement("logo").text());
    language = e.firstChildElement("language").text();
    copyright = e.firstChildElement("copyright").text();

    const QDomElement publisherElem = e.firstChildElement("publisher");
    publisher.name = publisherElem.attribute("name");
    publisher.url = KUrl(publisherElem.attribute("url"));

    const QDomElement lincenseElem = e.firstChildElement("license");
    license.name = lincenseElem.attribute("name");
    license.url = KUrl(lincenseElem.attribute("url"));

    for (QDomElement elemRes = e.firstChildElement("os"); !elemRes.isNull(); elemRes = elemRes.nextSiblingElement("os")) {
        oses << elemRes.text();
    }
}

void KGetMetalink::CommonData::save(QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();

    if (!copyright.isEmpty())
    {
        QDomElement elem = doc.createElement("copyright");
        QDomText text = doc.createTextNode(copyright);
        elem.appendChild(text);
        e.appendChild(elem);
    }
    if (!description.isEmpty())
    {
        QDomElement elem = doc.createElement("description");
        QDomText text = doc.createTextNode(description);
        elem.appendChild(text);
        e.appendChild(elem);
    }
    if (!identity.isEmpty())
    {
        QDomElement elem = doc.createElement("identity");
        QDomText text = doc.createTextNode(identity);
        elem.appendChild(text);
        e.appendChild(elem);
    }
    if (!language.isEmpty())
    {
        QDomElement elem = doc.createElement("language");
        QDomText text = doc.createTextNode(language);
        elem.appendChild(text);
        e.appendChild(elem);
    }
    if (!license.isEmpty())
    {
        QDomElement elem = doc.createElement("license");

        elem.setAttribute("url", license.url.url());
        elem.setAttribute("name", license.name);

        e.appendChild(elem);
    }
    if (!logo.isEmpty())
    {
        QDomElement elem = doc.createElement("logo");
        QDomText text = doc.createTextNode(logo.url());
        elem.appendChild(text);
        e.appendChild(elem);
    }
    if (!publisher.isEmpty())
    {
        QDomElement elem = doc.createElement("publisher");
        elem.setAttribute("url", publisher.url.url());
        elem.setAttribute("name", publisher.name);

        e.appendChild(elem);
    }
    if (!version.isEmpty())
    {
        QDomElement elem = doc.createElement("version");
        QDomText text = doc.createTextNode(version);
        elem.appendChild(text);
        e.appendChild(elem);
    }

    foreach (const QString &os, oses) {
        QDomElement elem = doc.createElement("os");
        QDomText text = doc.createTextNode(os);
        elem.appendChild(text);
        e.appendChild(elem);
    }
}

void KGetMetalink::CommonData::clear()
{
    identity.clear();
    version.clear();
    description.clear();
    oses.clear();
    logo.clear();
    language.clear();
    publisher.clear();
    copyright.clear();
    license.clear();
}

#ifdef HAVE_NEPOMUK
QHash<QUrl, Nepomuk::Variant> KGetMetalink::CommonData::properties() const
{
    //TODO what to do with identity?
    //TODO what uri for logo?
    //TODO what uri for publisher-url?
    //TODO what uri for license-url?
    QHash<QUrl, Nepomuk::Variant> data;

    HandleMetalink::addProperty(&data, "http://www.semanticdesktop.org/ontologies/2007/01/19/nie/#version", version);
    HandleMetalink::addProperty(&data, "http://www.semanticdesktop.org/ontologies/2007/01/19/nie/#description", description);
    if (oses.count()) {
        HandleMetalink::addProperty(&data, "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo/#OperatingSystem", oses.first());//TODO support all set oses!
    }
    HandleMetalink::addProperty(&data, "http://www.semanticdesktop.org/ontologies/nie/#language", language);
    HandleMetalink::addProperty(&data, "http://www.semanticdesktop.org/ontologies/2007/03/22/nco/#publisher", publisher.name);
    HandleMetalink::addProperty(&data, "http://www.semanticdesktop.org/ontologies/nie/#copyright", copyright);
    HandleMetalink::addProperty(&data, "http://www.semanticdesktop.org/ontologies/nie/#licenseType", license.name);

    return data;
}
#endif //HAVE_NEPOMUK

void KGetMetalink::Metaurl::load(const QDomElement &e)
{
    type = e.attribute("type").toLower();
    preference = e.attribute("preference").toInt();
    name = e.attribute("name");
    url = KUrl(e.text());
}

void KGetMetalink::Metaurl::save(QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();
    QDomElement metaurl = doc.createElement("metaurl");
    if (preference)
    {
        metaurl.setAttribute("preference", preference);
    }
    if (!name.isEmpty())
    {
        metaurl.setAttribute("name", name);
    }
    metaurl.setAttribute("type", type);

    QDomText text = doc.createTextNode(url.url());
    metaurl.appendChild(text);

    e.appendChild(metaurl);
}

bool KGetMetalink::Metaurl::isValid()
{
    return url.isValid() && !type.isEmpty();
}

void KGetMetalink::Metaurl::clear()
{
    type.clear();
    preference = 0;
    name.clear();
    url.clear();
}

bool KGetMetalink::Url::operator<(const KGetMetalink::Url &other) const
{
    bool smaller = this->preference < other.preference;

    if (!smaller && (this->preference == other.preference)) {
        QString countryCode = KGlobal::locale()->country();
        if (!countryCode.isEmpty()) {
            smaller = (other.location.toLower() == countryCode.toLower());
        }
    }
     return smaller;
}

void KGetMetalink::Url::load(const QDomElement &e)
{
    location = e.attribute("location").toLower();
    preference = e.attribute("preference").toInt();
    url = KUrl(e.text());
}

void KGetMetalink::Url::save(QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();
    QDomElement elem = doc.createElement("url");
    if (preference)
    {
        elem.setAttribute("preference", preference);
    }
    if (!location.isEmpty())
    {
        elem.setAttribute("location", location);
    }

    QDomText text = doc.createTextNode(url.url());
    elem.appendChild(text);

    e.appendChild(elem);
}

bool KGetMetalink::Url::isValid()
{
    bool valid = url.isValid();
    if (url.fileName().endsWith(QLatin1String(".torrent")))
    {
        valid = false;
    }
    else if (url.fileName().endsWith(QLatin1String(".metalink")) || url.fileName().endsWith(QLatin1String(".meta4")))
    {
        valid = false;
    }

    return valid;
}

void KGetMetalink::Url::clear()
{
    preference = 0;
    location.clear();
    url.clear();
}

void KGetMetalink::Resources::load(const QDomElement &e)
{
    for (QDomElement elem = e.firstChildElement("url"); !elem.isNull(); elem = elem.nextSiblingElement("url"))
    {
        Url url;
        url.load(elem);
        if (url.isValid())
        {
            urls.append(url);
        }
    }

    for (QDomElement elem = e.firstChildElement("metaurl"); !elem.isNull(); elem = elem.nextSiblingElement("metaurl"))
    {
        Metaurl metaurl;
        metaurl.load(elem);
        if (metaurl.isValid())
        {
            metaurls.append(metaurl);
        }
    }
}

void KGetMetalink::Resources::save(QDomElement &e) const
{
    foreach (const Metaurl &metaurl, metaurls)
    {
        metaurl.save(e);
    }

    foreach (const Url &url, urls)
    {
        url.save(e);
    }
}

void KGetMetalink::Resources::clear()
{
    urls.clear();
    metaurls.clear();
}

void KGetMetalink::Pieces::load(const QDomElement &e)
{
    type = addaptHashType(e.attribute("type"), true);
    length = e.attribute("length").toULongLong();

    QDomNodeList hashesList = e.elementsByTagName("hash");

    for (int i = 0; i < hashesList.count(); ++i)
    {
        QDomElement element = hashesList.at(i).toElement();
        hashes.append(element.text());
    }
}

void KGetMetalink::Pieces::save(QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();
    QDomElement pieces = doc.createElement("pieces");
    pieces.setAttribute("type", addaptHashType(type, false));
    pieces.setAttribute("length", length);

    for (int i = 0; i < hashes.size(); ++i)
    {
        QDomElement hash = doc.createElement("hash");
        QDomText text = doc.createTextNode(hashes.at(i));
        hash.appendChild(text);
        pieces.appendChild(hash);
    }

    e.appendChild(pieces);
}

void KGetMetalink::Pieces::clear()
{
    type.clear();
    length = 0;
    hashes.clear();
}

void KGetMetalink::Verification::load(const QDomElement &e)
{
    for (QDomElement elem = e.firstChildElement("hash"); !elem.isNull(); elem = elem.nextSiblingElement("hash")) {
        QString type = elem.attribute("type");
        const QString hash = elem.text();
        if (!type.isEmpty() && !hash.isEmpty()) {
            type = addaptHashType(type, true);
            hashes[type] = hash;
        }
    }

    for (QDomElement elem = e.firstChildElement("pieces"); !elem.isNull(); elem = elem.nextSiblingElement("pieces")) {
        Pieces piecesItem;
        piecesItem.load(elem);
        pieces.append(piecesItem);
    }

    for (QDomElement elem = e.firstChildElement("signature"); !elem.isNull(); elem = elem.nextSiblingElement("signature")) {
        const QString type = elem.attribute("type");
        const QString siganture = elem.text();
        if (!type.isEmpty() && !siganture.isEmpty()) {
            signatures[type] = siganture;
        }
    }
}

void KGetMetalink::Verification::save(QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();

    QHash<QString, QString>::const_iterator it;
    QHash<QString, QString>::const_iterator itEnd = hashes.constEnd();
    for (it = hashes.constBegin(); it != itEnd; ++it) {
        QDomElement hash = doc.createElement("hash");
        hash.setAttribute("type", addaptHashType(it.key(), false));
        QDomText text = doc.createTextNode(it.value());
        hash.appendChild(text);
        e.appendChild(hash);
    }

    foreach (const Pieces &item, pieces) {
        item.save(e);
    }

    itEnd = signatures.constEnd();
    for (it = signatures.constBegin(); it != itEnd; ++it) {
        QDomElement hash = doc.createElement("signature");
        hash.setAttribute("type", it.key());
        QDomText text = doc.createTextNode(it.value());
        hash.appendChild(text);
        e.appendChild(hash);
    }
}

void KGetMetalink::Verification::clear()
{
    hashes.clear();
    pieces.clear();
}

bool KGetMetalink::File::isValid() const
{
    return !name.isEmpty() && resources.isValid();
}

void KGetMetalink::File::load(const QDomElement &e)
{
    data.load(e);

    name = e.attribute("name");
    size = e.firstChildElement("size").text().toULongLong();

    verification.load(e);
    resources.load(e);
}

void KGetMetalink::File::save(QDomElement &e) const
{
    if (isValid())
    {
        QDomDocument doc = e.ownerDocument();
        QDomElement file = doc.createElement("file");
        file.setAttribute("name", name);

        if (size)
        {
            QDomElement elem = doc.createElement("size");
            QDomText text = doc.createTextNode(QString::number(size));
            elem.appendChild(text);
            file.appendChild(elem);
        }

        data.save(file);
        resources.save(file);
        verification.save(file);

        e.appendChild(file);
    }
}

void KGetMetalink::File::clear()
{
    name.clear();
    verification.clear();
    size = 0;
    data.clear();
    resources.clear();
}

#ifdef HAVE_NEPOMUK
QHash<QUrl, Nepomuk::Variant> KGetMetalink::File::properties() const
{
    return data.properties();
}
#endif //HAVE_NEPOMUK

bool KGetMetalink::Files::isValid() const
{
    bool isValid = !files.empty();
    foreach (const File &file, files)
    {
        isValid &= file.isValid();
    }

    return isValid;
}

void KGetMetalink::Files::load(const QDomElement &e)
{
    for (QDomElement elem = e.firstChildElement("file"); !elem.isNull(); elem = elem.nextSiblingElement("file"))
    {
        File file;
        file.load(elem);
        files.append(file);
    }
}

void KGetMetalink::Files::save(QDomElement &e) const
{
    if (e.isNull())
    {
        return;
    }

    foreach (const File &file, files)
    {
        file.save(e);
    }
}

void KGetMetalink::Files::clear()
{
    files.clear();
}

bool KGetMetalink::Metalink::isValid() const
{
    return files.isValid();
}

// #ifdef HAVE_NEPOMUK
// QHash<QUrl, Nepomuk::Variant> KGetMetalink::Files::properties() const
// {
//     return data.properties();
// }
// #endif //HAVE_NEPOMUK

void KGetMetalink::Metalink::load(const QDomElement &e)
{
    QDomDocument doc = e.ownerDocument();
    const QDomElement metalink = doc.firstChildElement("metalink");


    dynamic = (metalink.firstChildElement("dynamic").text() == "true");
    xmlns = metalink.attribute("xmlns");
    origin = KUrl(metalink.firstChildElement("origin").text());
    generator = metalink.firstChildElement("generator").text();
    updated.setData(metalink.firstChildElement("updated").text());
    published.setData(metalink.firstChildElement("published").text());
    updated.setData(metalink.firstChildElement("updated").text());


    files.load(e);
}

QDomDocument KGetMetalink::Metalink::save() const
{
    QDomDocument doc;
    QDomProcessingInstruction header = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(header);

    QDomElement metalink = doc.createElement("metalink");
    metalink.setAttribute("xmlns", "urn:ietf:params:xml:ns:metalink"); //the xmlns value is ignored, instead the data format described in the specification is always used

    QDomElement elem = doc.createElement("generator");
    QDomText text = doc.createTextNode(Metalink::KGET_DESCRIPTION); //the set generator is ignored, instead when saving KGET is always used
    elem.appendChild(text);
    metalink.appendChild(elem);

    if (!origin.isEmpty()) {
        QDomElement elem = doc.createElement("origin");
        QDomText text = doc.createTextNode(origin.url());
        elem.appendChild(text);
        metalink.appendChild(elem);
    }
    if (published.isValid()) {
        QDomElement elem = doc.createElement("published");
        QDomText text = doc.createTextNode(published.toString());
        elem.appendChild(text);
        metalink.appendChild(elem);
    }
    if (dynamic) {
        QDomElement elem = doc.createElement("dynamic");
        QDomText text = doc.createTextNode("true");
        elem.appendChild(text);
        metalink.appendChild(elem);
    }
    if (updated.isValid()) {
        QDomElement elem = doc.createElement("updated");
        QDomText text = doc.createTextNode(updated.toString());
        elem.appendChild(text);
        metalink.appendChild(elem);
    }

    files.save(metalink);

    doc.appendChild(metalink);

    return doc;
}

void KGetMetalink::Metalink::clear()
{
    dynamic = false;
    xmlns.clear();
    published.clear();
    origin.clear();
    generator.clear();
    updated.clear();
    files.clear();
}

KGetMetalink::Metalink_v3::Metalink_v3()
{
}

KGetMetalink::Metalink KGetMetalink::Metalink_v3::metalink()
{
    return m_metalink;
}

void KGetMetalink::Metalink_v3::setMetalink(const KGetMetalink::Metalink &metalink)
{
    m_metalink = metalink;
}

void KGetMetalink::Metalink_v3::load(const QDomElement &e)
{
    QDomDocument doc = e.ownerDocument();
    const QDomElement metalinkDom = doc.firstChildElement("metalink");

    m_metalink.dynamic = (metalinkDom.attribute("type") == "dynamic");
    m_metalink.origin = KUrl(metalinkDom.attribute("origin"));
    m_metalink.generator = metalinkDom.attribute("generator");
    m_metalink.published = parseDateConstruct(metalinkDom.attribute("pubdate"));
    m_metalink.updated = parseDateConstruct(metalinkDom.attribute("refreshdate"));

    parseFiles(metalinkDom);
}

void KGetMetalink::Metalink_v3::parseFiles(const QDomElement &e)
{
    //here we assume that the CommonData set in metalink is for every file in the metalink
    CommonData data;
    data = parseCommonData(e);

    const QDomElement filesElem = e.firstChildElement("files");
    CommonData filesData = parseCommonData(filesElem);
    
    inheritCommonData(data, &filesData);

    for (QDomElement elem = filesElem.firstChildElement("file"); !elem.isNull(); elem = elem.nextSiblingElement("file")) {
        File file;
        file.name = elem.attribute("name");
        file.size = elem.firstChildElement("size").text().toULongLong();

        file.data = parseCommonData(elem);
        inheritCommonData(filesData, &file.data);

        file.resources = parseResources(elem);

        //load the verification information
        QDomElement veriE = elem.firstChildElement("verification");

        for (QDomElement elemVer = veriE.firstChildElement("hash"); !elemVer.isNull(); elemVer = elemVer.nextSiblingElement("hash")) {
            QString type = elemVer.attribute("type");
            QString hash = elemVer.text();
            if (!type.isEmpty() && !hash.isEmpty()) {
                type = addaptHashType(type, true);
                file.verification.hashes[type] = hash;
            }
        }

        for (QDomElement elemVer = veriE.firstChildElement("pieces"); !elemVer.isNull(); elemVer = elemVer.nextSiblingElement("pieces")) {
            Pieces piecesItem;
            piecesItem.load(elemVer);
            file.verification.pieces.append(piecesItem);
        }

         for (QDomElement elemVer = veriE.firstChildElement("signature"); !elemVer.isNull(); elemVer = elemVer.nextSiblingElement("signature")) {
            const QString type = elemVer.attribute("type");
            const QString signature = elemVer.text();
            if (!type.isEmpty() && !signature.isEmpty()) {
                file.verification.signatures[type] = signature;
            }
        }

        m_metalink.files.files.append(file);
    }
}

KGetMetalink::CommonData KGetMetalink::Metalink_v3::parseCommonData(const QDomElement &e)
{
    CommonData data;

    data.load(e);

    const QDomElement publisherElem = e.firstChildElement("publisher");
    data.publisher.name = publisherElem.firstChildElement("name").text();
    data.publisher.url = KUrl(publisherElem.firstChildElement("url").text());

    const QDomElement lincenseElem = e.firstChildElement("license");
    data.license.name = lincenseElem.firstChildElement("name").text();
    data.license.url = KUrl(lincenseElem.firstChildElement("url").text());

    return data;
}

void KGetMetalink::Metalink_v3::inheritCommonData(const KGetMetalink::CommonData &ancestor, KGetMetalink::CommonData *inheritor)
{
    if (!inheritor) {
        return;
    }
    
    //ensure that inheritance works
    if (inheritor->identity.isEmpty()) {
        inheritor->identity = ancestor.identity;
    }
    if (inheritor->version.isEmpty()) {
        inheritor->version = ancestor.version;
    }
    if (inheritor->description.isEmpty()) {
        inheritor->description = ancestor.description;
    }
    if (inheritor->oses.isEmpty()) {
        inheritor->oses = ancestor.oses;
    }
    if (inheritor->logo.isEmpty()) {
        inheritor->logo = ancestor.logo;
    }
    if (inheritor->language.isEmpty()) {
        inheritor->language = ancestor.language;
    }
    if (inheritor->copyright.isEmpty()) {
        inheritor->copyright = ancestor.copyright;
    }
    if (inheritor->publisher.isEmpty()) {
        inheritor->publisher = ancestor.publisher;
    }
    if (inheritor->license.isEmpty()) {
        inheritor->license = ancestor.license;
    }
}

KGetMetalink::Resources KGetMetalink::Metalink_v3::parseResources(const QDomElement &e)
{
    Resources resources;

    QDomElement res = e.firstChildElement("resources");
    for (QDomElement elemRes = res.firstChildElement("url"); !elemRes.isNull(); elemRes = elemRes.nextSiblingElement("url")) {
        const QString location = elemRes.attribute("location");
        int preference = elemRes.attribute("preference").toInt();
        if (preference > 100) {
            preference = 100;
        }
        const KUrl link = KUrl(elemRes.text());
        QString type;

        if (link.fileName().endsWith(QLatin1String(".torrent"))) {
            type = "torrent";
        }

        if (type.isEmpty()) {
            Url url;
            url.location = location;
            url.preference = preference;
            url.url = link;
            url.load(elemRes);
            if (url.isValid()) {
                resources.urls.append(url);
            }
        } else {
            //it might be a metaurl
            Metaurl metaurl;
            metaurl.preference = preference;
            metaurl.url = link;
            metaurl.type = type;
            if (metaurl.isValid()) {
                resources.metaurls.append(metaurl);
            }
        }
    }

    return resources;
}

KGetMetalink::DateConstruct KGetMetalink::Metalink_v3::parseDateConstruct(const QString &data)
{
    DateConstruct dateConstruct;

    if (data.isEmpty()){
        return dateConstruct;
    }

    kDebug(5001) << "Parsing" << data;

    QString temp = data;
    QDateTime dateTime;
    QTime timeZoneOffset;

    //Date according to RFC 822, the year with four characters preferred
    //e.g.: "Mon, 15 May 2006 00:00:01 GMT", "Fri, 01 Apr 2009 00:00:01 +1030"
    
    //find the date
    const QString weekdayExp = "ddd, ";
    const bool weekdayIncluded = (temp.indexOf(',') == 3);
    int startPosition = (weekdayIncluded ? weekdayExp.length() : 0);
    const QString dayMonthExp = "dd MMM ";
    const QString yearExp = "yy";

    QString exp = dayMonthExp + yearExp + yearExp;
    int length = exp.length();

    QDate date = QDate::fromString(temp.mid(startPosition, length), exp);
    if (!date.isValid()) {
        exp = dayMonthExp + yearExp;
        length = exp.length();
        date = QDate::fromString(temp.mid(startPosition, length), exp);
        if (!date.isValid()) {
            return dateConstruct;
        }
    }

    //find the time
    dateTime.setDate(date);
    temp = temp.mid(startPosition);
    temp = temp.mid(length + 1);//also remove the space

    const QString hourExp = "hh";
    const QString minuteExp = "mm";
    const QString secondExp = "ss";

    exp = hourExp + ':' + minuteExp + ':' + secondExp;
    length = exp.length();
    QTime time = QTime::fromString(temp.left(length), exp);
    if (!time.isValid()) {
        exp = hourExp + ':' + minuteExp;
        length = exp.length();
        time = QTime::fromString(temp.left(length), exp);
        if (!time.isValid()) {
            return dateConstruct;
        }
    }
    dateTime.setTime(time);
    
    //find the offset
    temp = temp.mid(length + 1);//also remove the space
    bool negativeOffset = false;

    if (temp.length() == 3) { //e.g. GMT
        KTimeZone timeZone = KSystemTimeZones::readZone(temp);
        if (timeZone.isValid()) {
            int offset = timeZone.currentOffset();
            negativeOffset = (offset < 0);
            timeZoneOffset = QTime(0, 0, 0);
            timeZoneOffset = timeZoneOffset.addSecs(qAbs(offset));
        }
    } else if (temp.length() == 5) { //e.g. +1030
        negativeOffset = (temp[0] == '-');
        timeZoneOffset = QTime::fromString(temp.mid(1,4), "hhmm");
    }

    dateConstruct.setData(dateTime, timeZoneOffset, negativeOffset);

    return dateConstruct;
}

QDomDocument KGetMetalink::Metalink_v3::save() const
{
    QDomDocument doc;
    QDomProcessingInstruction header = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(header);

    QDomElement metalink = doc.createElement("metalink");
    metalink.setAttribute("xmlns", "http://www.metalinker.org/");
    metalink.setAttribute("version", "3.0");
    metalink.setAttribute("type", (m_metalink.dynamic ? "dynamic" : "static"));
    metalink.setAttribute("generator", Metalink::KGET_DESCRIPTION); //the set generator is ignored, instead when saving KGET is always used

    if (m_metalink.published.isValid()) {
        metalink.setAttribute("pubdate", dateConstructToString(m_metalink.published));
    }
    if (m_metalink.updated.isValid()) {
        metalink.setAttribute("refreshdate", dateConstructToString(m_metalink.updated));
    }
    if (!m_metalink.origin.isEmpty()) {
        metalink.setAttribute("origin", m_metalink.origin.url());
    }

    saveFiles(metalink);

    doc.appendChild(metalink);

    return doc;
}

void KGetMetalink::Metalink_v3::saveFiles(QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();
    QDomElement filesElem = doc.createElement("files");

    foreach (const File &file, m_metalink.files.files) {
        QDomElement elem = doc.createElement("file");
        elem.setAttribute("name", file.name);

        QDomElement size = doc.createElement("size");
        QDomText text = doc.createTextNode(QString::number(file.size));
        size.appendChild(text);
        elem.appendChild(size);

        saveCommonData(file.data, elem);
        saveResources(file.resources, elem);
        saveVerification(file.verification, elem);

        filesElem.appendChild(elem);
    }

    e.appendChild(filesElem);
}

void KGetMetalink::Metalink_v3::saveResources(const Resources &resources, QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();
    QDomElement res = doc.createElement("resources");

    foreach (const Url &url, resources.urls) {
        QDomElement elem = doc.createElement("url");
        if (url.preference) {
            elem.setAttribute("preference", url.preference);//TODO convert!!!
        }
        if (!url.location.isEmpty()) {
            elem.setAttribute("location", url.location);
        }

        QDomText text = doc.createTextNode(url.url.url());
        elem.appendChild(text);

        res.appendChild(elem);
    }

    foreach (const Metaurl &metaurl, resources.metaurls) {
        if (metaurl.type == "torrent") {
            QDomElement elem = doc.createElement("url");
            if (metaurl.preference) {
                elem.setAttribute("preference", metaurl.preference);//TODO convert!!!
                elem.setAttribute("type", "bittorrent");
            }

            QDomText text = doc.createTextNode(metaurl.url.url());
            elem.appendChild(text);

            res.appendChild(elem);
        }
    }

    e.appendChild(res);
}

void KGetMetalink::Metalink_v3::saveVerification(const KGetMetalink::Verification &verification, QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();
    QDomElement veri = doc.createElement("verification");

    QHash<QString, QString>::const_iterator it;
    QHash<QString, QString>::const_iterator itEnd = verification.hashes.constEnd();
    for (it = verification.hashes.constBegin(); it != itEnd; ++it) {
        QDomElement elem = doc.createElement("hash");
        elem.setAttribute("type", it.key());
        QDomText text = doc.createTextNode(it.value());
        elem.appendChild(text);

        veri.appendChild(elem);
    }

    foreach (const Pieces &pieces, verification.pieces) {
        QDomElement elem = doc.createElement("pieces");
        elem.setAttribute("type", pieces.type);
        elem.setAttribute("length", QString::number(pieces.length));

        for (int i = 0; i < pieces.hashes.count(); ++i) {
            QDomElement hash = doc.createElement("hash");
            hash.setAttribute("piece", i);
            QDomText text = doc.createTextNode(pieces.hashes.at(i));
            hash.appendChild(text);

            elem.appendChild(hash);
        }
        veri.appendChild(elem);
    }

    itEnd = verification.signatures.constEnd();
    for (it = verification.signatures.constBegin(); it != itEnd; ++it) {
        QDomElement elem = doc.createElement("signature");
        elem.setAttribute("type", it.key());
        QDomText text = doc.createTextNode(it.value());
        elem.appendChild(text);

        veri.appendChild(elem);
    }

    e.appendChild(veri);
}

void KGetMetalink::Metalink_v3::saveCommonData(const KGetMetalink::CommonData &data, QDomElement &e) const
{
    QDomDocument doc = e.ownerDocument();

    CommonData commonData = data;

    if (!commonData.publisher.isEmpty()) {
        QDomElement elem = doc.createElement("publisher");
        QDomElement elemName = doc.createElement("name");
        QDomElement elemUrl = doc.createElement("url");

        QDomText text = doc.createTextNode(commonData.publisher.name);
        elemName.appendChild(text);
        elem.appendChild(elemName);

        text = doc.createTextNode(commonData.publisher.url.url());
        elemUrl.appendChild(text);
        elem.appendChild(elemUrl);

        e.appendChild(elem);

        commonData.publisher.clear();
    }
    if (!commonData.license.isEmpty()) {
        QDomElement elem = doc.createElement("license");
        QDomElement elemName = doc.createElement("name");
        QDomElement elemUrl = doc.createElement("url");

        QDomText text = doc.createTextNode(commonData.license.name);
        elemName.appendChild(text);
        elem.appendChild(elemName);

        text = doc.createTextNode(commonData.license.url.url());
        elemUrl.appendChild(text);
        elem.appendChild(elemUrl);

        e.appendChild(elem);

        commonData.license.clear();
    }

    if (commonData.oses.count() > 1) {//only one OS can be set in 3.0
        commonData.oses.clear();
    }

    commonData.save(e);
}

QString KGetMetalink::Metalink_v3::dateConstructToString(const KGetMetalink::DateConstruct &date) const
{
    QString dateString;
    if (!date.isValid()) {
        return dateString;
    }

    //"Fri, 01 Apr 2009 00:00:01 +1030"
    dateString += DateConstruct::WEEKDAYS[date.dateTime.date().dayOfWeek()];
    dateString += date.dateTime.toString(", dd ");
    dateString += DateConstruct::MONTHS[date.dateTime.date().month()];
    dateString += date.dateTime.toString(" yyyy hh:mm:ss ");

    if (date.timeZoneOffset.isValid()) {
        dateString += (date.negativeOffset ? '-' : '+');
        dateString += date.timeZoneOffset.toString("hhmm");
    } else {
        dateString += "+0000";
    }

    return dateString;
}


bool KGetMetalink::HandleMetalink::load(const KUrl &destination, KGetMetalink::Metalink *metalink)
{
    QFile file(destination.pathOrUrl());
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        file.close();
        return false;
    }
    file.close();

    QDomElement root = doc.documentElement();
    if (root.attribute("xmlns") == "urn:ietf:params:xml:ns:metalink")
    {
        metalink->load(root);
        return true;
    }
    else if ((root.attribute("xmlns") == "http://www.metalinker.org/") || (root.attribute("version") == "3.0"))
    {
        Metalink_v3 metalink_v3;
        metalink_v3.load(root);
        *metalink = metalink_v3.metalink();
        return true;
    }

    return false;
}

bool KGetMetalink::HandleMetalink::load(const QByteArray &data, KGetMetalink::Metalink *metalink)
{
    if (data.isNull())
    {
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(data))
    {
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.attribute("xmlns") == "urn:ietf:params:xml:ns:metalink")
    {
        metalink->load(root);
        return true;
    }
    else if ((root.attribute("xmlns") == "http://www.metalinker.org/") || (root.attribute("version") == "3.0"))
    {
        Metalink_v3 metalink_v3;
        metalink_v3.load(root);
        *metalink = metalink_v3.metalink();
        return true;
    }

    return false;
}

bool KGetMetalink::HandleMetalink::save(const KUrl &destination, KGetMetalink::Metalink *metalink)
{
    QFile file(destination.pathOrUrl());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDomDocument doc;
    QString fileName = destination.fileName();
    if (fileName.endsWith("meta4")) {
        doc = metalink->save();
    } else if (fileName.endsWith("metalink")) {
        Metalink_v3 metalink_v3;
        metalink_v3.setMetalink(*metalink);
        doc = metalink_v3.save();
    } else {
        file.close();
        return false;
    }

    QTextStream stream(&file);
    doc.save(stream, 2);
    file.close();

    return true;
}

#ifdef HAVE_NEPOMUK
void KGetMetalink::HandleMetalink::addProperty(QHash<QUrl, Nepomuk::Variant> *data, const QByteArray &uriBa, const QString &value)
{
    if (data && !uriBa.isEmpty() && !value.isEmpty())
    {
        const QUrl uri = QUrl::fromEncoded(uriBa, QUrl::StrictMode);
        (*data)[uri] = Nepomuk::Variant(value);
    }
}
#endif //HAVE_NEPOMUK