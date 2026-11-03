#include "core/Markdown.h"

#include <QByteArray>

#include <md4c-html.h>
#include <md4c.h>

namespace {

void appendOutput(const MD_CHAR* data, MD_SIZE size, void* userData) {
    static_cast<QByteArray*>(userData)->append(data, static_cast<qsizetype>(size));
}

}  // namespace

namespace Markdown {

QString toHtml(const QString& markdown) {
    const QByteArray input = markdown.toUtf8();
    QByteArray html;
    const int status = md_html(input.constData(), static_cast<MD_SIZE>(input.size()), appendOutput,
                               &html, MD_DIALECT_GITHUB | MD_FLAG_NOHTML, 0);
    return status == 0 ? QString::fromUtf8(html) : QString();
}

}  // namespace Markdown
