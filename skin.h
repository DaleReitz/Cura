#ifndef SKIN_H
#define SKIN_H

void generateSkins(int layerNr, SliceDataStorage& storage, int extrusionWidth, int downSkinCount, int upSkinCount)
{
    SliceLayer* layer = &storage.layers[layerNr];

    for(unsigned int partNr=0; partNr<layer->parts.size(); partNr++)
    {
        SliceLayerPart* part = &layer->parts[partNr];
        
        ClipperLib::Clipper downskinClipper;
        ClipperLib::Clipper upskinClipper;
        downskinClipper.AddPolygons(part->insets[part->insets.size() - 1], ClipperLib::ptSubject);
        upskinClipper.AddPolygons(part->insets[part->insets.size() - 1], ClipperLib::ptSubject);
        
        if (part->insets.size() > 1)
        {
            ClipperLib::Clipper thinWallClipper;
            Polygons temp;
            ClipperLib::OffsetPolygons(part->insets[1], temp, extrusionWidth, ClipperLib::jtSquare, 2, false);
            thinWallClipper.AddPolygons(part->insets[0], ClipperLib::ptSubject);
            thinWallClipper.AddPolygons(temp, ClipperLib::ptClip);
            thinWallClipper.Execute(ClipperLib::ctDifference, temp);
            downskinClipper.AddPolygons(temp, ClipperLib::ptSubject);
            upskinClipper.AddPolygons(temp, ClipperLib::ptSubject);
        }
        
        if (int(layerNr - downSkinCount) >= 0)
        {
            SliceLayer* layer2 = &storage.layers[layerNr - downSkinCount];
            for(unsigned int partNr2=0; partNr2<layer2->parts.size(); partNr2++)
            {
                downskinClipper.AddPolygons(layer2->parts[partNr2].insets[layer2->parts[partNr2].insets.size() - 1], ClipperLib::ptClip);
            }
        }
        if (int(layerNr + upSkinCount) < (int)storage.layers.size())
        {
            SliceLayer* layer2 = &storage.layers[layerNr + upSkinCount];
            for(unsigned int partNr2=0; partNr2<layer2->parts.size(); partNr2++)
            {
                upskinClipper.AddPolygons(layer2->parts[partNr2].insets[layer2->parts[partNr2].insets.size() - 1], ClipperLib::ptClip);
            }
        }
        
        ClipperLib::Polygons downSkin;
        ClipperLib::Polygons upSkin;
        downskinClipper.Execute(ClipperLib::ctDifference, downSkin);
        upskinClipper.Execute(ClipperLib::ctDifference, upSkin);
        
        {
            ClipperLib::Clipper skinCombineClipper;
            skinCombineClipper.AddPolygons(downSkin, ClipperLib::ptSubject);
            skinCombineClipper.AddPolygons(upSkin, ClipperLib::ptClip);
            skinCombineClipper.Execute(ClipperLib::ctUnion, part->skinOutline);
        }

        double minAreaSize = 3.0;//(2 * M_PI * (double(config.extrusionWidth) / 1000.0) * (double(config.extrusionWidth) / 1000.0)) * 3;
        for(unsigned int i=0; i<part->skinOutline.size(); i++)
        {
            double area = fabs(ClipperLib::Area(part->skinOutline[i])) / 1000.0 / 1000.0;
            if (area < minAreaSize) /* Only create an up/down skin if the area is large enough. So you do not create tiny blobs of "trying to fill" */
            {
                part->skinOutline.erase(part->skinOutline.begin() + i);
                i -= 1;
            }
        }
    }
}

void generateSparse(int layerNr, SliceDataStorage& storage, int downSkinCount, int upSkinCount)
{
    SliceLayer* layer = &storage.layers[layerNr];

    for(unsigned int partNr=0; partNr<layer->parts.size(); partNr++)
    {
        SliceLayerPart* part = &layer->parts[partNr];
        
        ClipperLib::Clipper downskinClipper;
        ClipperLib::Clipper upskinClipper;
        downskinClipper.AddPolygons(part->insets[part->insets.size() - 1], ClipperLib::ptSubject);
        upskinClipper.AddPolygons(part->insets[part->insets.size() - 1], ClipperLib::ptSubject);
        if (int(layerNr - downSkinCount) >= 0)
        {
            SliceLayer* layer2 = &storage.layers[layerNr - downSkinCount];
            for(unsigned int partNr2=0; partNr2<layer2->parts.size(); partNr2++)
            {
                if (layer2->parts[partNr2].insets.size() > 1)
                    downskinClipper.AddPolygons(layer2->parts[partNr2].insets[layer2->parts[partNr2].insets.size() - 2], ClipperLib::ptClip);
            }
        }
        if (int(layerNr + upSkinCount) < (int)storage.layers.size())
        {
            SliceLayer* layer2 = &storage.layers[layerNr + upSkinCount];
            for(unsigned int partNr2=0; partNr2<layer2->parts.size(); partNr2++)
            {
                if (layer2->parts[partNr2].insets.size() > 1)
                    upskinClipper.AddPolygons(layer2->parts[partNr2].insets[layer2->parts[partNr2].insets.size() - 2], ClipperLib::ptClip);
            }
        }
        
        ClipperLib::Polygons downSkin;
        ClipperLib::Polygons upSkin;
        ClipperLib::Polygons result;
        downskinClipper.Execute(ClipperLib::ctDifference, downSkin);
        upskinClipper.Execute(ClipperLib::ctDifference, upSkin);
        
        {
            ClipperLib::Clipper skinClipper;
            skinClipper.AddPolygons(downSkin, ClipperLib::ptSubject);
            skinClipper.AddPolygons(upSkin, ClipperLib::ptClip);
            skinClipper.Execute(ClipperLib::ctUnion, result);
        }

        double minAreaSize = 3.0;//(2 * M_PI * (double(config.extrusionWidth) / 1000.0) * (double(config.extrusionWidth) / 1000.0)) * 3;
        for(unsigned int i=0; i<result.size(); i++)
        {
            double area = fabs(ClipperLib::Area(result[i])) / 1000.0 / 1000.0;
            if (area < minAreaSize) /* Only create an up/down skin if the area is large enough. So you do not create tiny blobs of "trying to fill" */
            {
                result.erase(result.begin() + i);
                i -= 1;
            }
        }
        
        ClipperLib::Clipper sparseClipper;
        sparseClipper.AddPolygons(part->insets[part->insets.size() - 1], ClipperLib::ptSubject);
        sparseClipper.AddPolygons(result, ClipperLib::ptClip);
        sparseClipper.Execute(ClipperLib::ctDifference, part->sparseOutline);
    }
}

#endif//SKIN_H
