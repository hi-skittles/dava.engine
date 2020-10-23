#ifndef __QUICKED_PACKAGE_VISITOR_H__
#define __QUICKED_PACKAGE_VISITOR_H__

class PackageNode;
class ControlNode;
class ImportedPackagesNode;
class PackageControlsNode;
class StyleSheetsNode;
class StyleSheetNode;

class PackageVisitor
{
public:
    PackageVisitor();
    virtual ~PackageVisitor();

    virtual void VisitPackage(PackageNode* node) = 0;
    virtual void VisitImportedPackages(ImportedPackagesNode* node) = 0;
    virtual void VisitControls(PackageControlsNode* node) = 0;
    virtual void VisitControl(ControlNode* node) = 0;
    virtual void VisitStyleSheets(StyleSheetsNode* node) = 0;
    virtual void VisitStyleSheet(StyleSheetNode* node) = 0;
};

#endif // __QUICKED_PACKAGE_VISITOR_H__
