/*
 *  Copyright (c) 2014 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_heightmap_import.h"

#include <ctype.h>

#include <QApplication>
#include <QFile>
#include <qendian.h>
#include <QCursor>

#include <kapplication.h>
#include <kpluginfactory.h>

#include <kio/netaccess.h>
#include <kio/deletejob.h>

#include <KoFilterManager.h>
#include <KoColorSpaceRegistry.h>
#include <KoFilterChain.h>
#include <KoColorModelStandardIds.h>
#include <KoColorSpace.h>
#include <KoColorSpaceTraits.h>

#include <kis_debug.h>
#include <kis_doc2.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_transaction.h>
#include <kis_iterator_ng.h>
#include <kis_random_accessor_ng.h>
#include <kis_config.h>

#include "ui_kis_wdg_options_heightmap.h"

K_PLUGIN_FACTORY(HeightMapImportFactory, registerPlugin<KisHeightMapImport>();)
K_EXPORT_PLUGIN(HeightMapImportFactory("krita"))

KisHeightMapImport::KisHeightMapImport(QObject *parent, const QVariantList &) : KoFilter(parent)
{
}

KisHeightMapImport::~KisHeightMapImport()
{
}

KoFilter::ConversionStatus KisHeightMapImport::convert(const QByteArray& from, const QByteArray& to)
{
    Q_UNUSED(from);

    dbgFile << "Importing using HeightMapImport!";

    if (to != "application/x-krita")
        return KoFilter::BadMimeType;

    KisDoc2 * doc = dynamic_cast<KisDoc2*>(m_chain -> outputDocument());

    if (!doc)
        return KoFilter::NoDocumentCreated;

    QString filename = m_chain -> inputFile();

    if (filename.isEmpty()) {
        return KoFilter::FileNotFound;
    }

    KUrl url(filename);


    dbgFile << "Import: " << url;
    if (url.isEmpty())
        return KoFilter::FileNotFound;

    if (!url.isLocalFile()) {
        return KoFilter::FileNotFound;
    }

    kapp->restoreOverrideCursor();

    KDialog* kdb = new KDialog(0);
    kdb->setWindowTitle(i18n("R16 HeightMap Import Options"));
    kdb->setButtons(KDialog::Ok | KDialog::Cancel);

    Ui::WdgOptionsHeightMap optionsHeightMap;

    QWidget* wdg = new QWidget(kdb);
    optionsHeightMap.setupUi(wdg);

    kdb->setMainWidget(wdg);

    QString filterConfig = KisConfig().exportConfiguration("HeightMap");
    KisPropertiesConfiguration cfg;
    cfg.fromXML(filterConfig);

    QFile f(url.toLocalFile());

    int w = 0;
    int h = 0;

    if (!f.exists()) {
        doc->setErrorMessage(i18n("File does not exist."));
        return KoFilter::CreationError;
    }

    if (!f.isOpen()) {
        f.open(QFile::ReadOnly);
    }

    optionsHeightMap.intSize->setValue(0);
    int endianness = cfg.getInt("endianness", 0);
    if (endianness == 0) {
        optionsHeightMap.radioMac->setChecked(true);
    }
    else {
        optionsHeightMap.radioPC->setChecked(true);
    }

    if (!m_chain->manager()->getBatchMode()) {
        if (kdb->exec() == QDialog::Rejected) {
            return KoFilter::OK; // FIXME Cancel doesn't exist :(
        }
    }

    w = h = optionsHeightMap.intSize->value();

    if ((w * h * 2) != f.size()) {
        doc->setErrorMessage(i18n("Source file is not the right size for the specified width and height."));
        return KoFilter::WrongFormat;
    }


    QDataStream::ByteOrder bo = QDataStream::LittleEndian;
    cfg.setProperty("endianness", 1);
    if (optionsHeightMap.radioMac->isChecked()) {
        bo = QDataStream::BigEndian;
        cfg.setProperty("endianness", 0);
    }
    KisConfig().setExportConfiguration("HeightMap", cfg);

    doc->prepareForImport();

    QDataStream s(&f);
    s.setByteOrder(bo);

    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->colorSpace(GrayAColorModelID.id(), Integer16BitsColorDepthID.id(), 0);
    KisImageSP image = new KisImage(doc->createUndoStore(), w, h, colorSpace, "imported heightmap");
    KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), 255);

    KisRandomAccessorSP it = layer->paintDevice()->createRandomAccessorNG(0, 0);
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            it->moveTo(i, j);
            quint16 pixel;
            s >> pixel;
            KoGrayU16Traits::setGray(it->rawData(), pixel);
            KoGrayU16Traits::setOpacity(it->rawData(), OPACITY_OPAQUE_F, 1);
        }
    }

    image->addNode(layer.data(), image->rootLayer().data());
    doc->setCurrentImage(image);
    return KoFilter::OK;
}

