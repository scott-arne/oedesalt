#include <gtest/gtest.h>

#include "oedesalt/Desalter.h"
#include "oedesalt/Error.h"
#include "oedesalt/oedesalt.h"

#include <oechem.h>

#include <fstream>
#include <string>
#include <vector>

namespace OEDESALT {
namespace test {
namespace {

OEChem::OEGraphMol MolFromSmiles(const std::string& smiles) {
    OEChem::OEGraphMol mol;
    EXPECT_TRUE(OEChem::OESmilesToMol(mol, smiles));
    return mol;
}

std::string CanonicalSmiles(const OEChem::OEMolBase& mol) {
    return OEChem::OEMolToSmiles(mol);
}

// A tiny inline salt file: halide + sodium, whole-fragment convention.
std::string WriteTempSaltFile(const std::string& body) {
    const std::string path = std::string(::testing::TempDir()) + "/oedesalt_test_salts.smarts";
    std::ofstream out(path);
    out << body;
    out.close();
    return path;
}

const char* kMiniSalts =
    "// mini salt set\n"
    "[F,Cl,Br,I]        Halides\n"
    "[Li,Na,K,Rb,Cs]    Alkali metals\n"
    "\n"
    "C(=O)O             Carboxylate counterion\n";

const char* kMiniSolvents =
    "// mini solvent set\n"
    "O                  Water\n";

Desalter MakeMiniDesalter() {
    return Desalter(load_salt_patterns(WriteTempSaltFile(kMiniSalts)));
}

}  // namespace

TEST(Version, MacrosDefined) {
    EXPECT_GE(OEDESALT_VERSION_MAJOR, 0);
    EXPECT_GE(OEDESALT_VERSION_MINOR, 0);
    EXPECT_GE(OEDESALT_VERSION_PATCH, 0);
}

TEST(Desalter, LoadsPatternsIgnoringCommentsAndBlanks) {
    const auto patterns = load_salt_patterns(WriteTempSaltFile(kMiniSalts));
    EXPECT_EQ(patterns.size(), 3u);
}

TEST(Desalter, CapturesMultiWordNameAsLineRemainder) {
    const std::string path = WriteTempSaltFile("[Cr,Mn,Fe]   Common transition / post-tx metals\n");
    const auto patterns = load_salt_patterns(path);
    ASSERT_EQ(patterns.size(), 1u);
    EXPECT_EQ(patterns[0].name, "Common transition / post-tx metals");
}

TEST(Desalter, MalformedSmartsThrowsNamingFileAndLine) {
    const std::string path = WriteTempSaltFile("// comment\n[Cl        Halide\n");
    try {
        load_salt_patterns(path);
        FAIL() << "Expected InvalidPatternError to be thrown";
    } catch (const InvalidPatternError& exc) {
        const std::string message(exc.what());
        EXPECT_NE(message.find(":2:"), std::string::npos)
            << "Error message should contain line number ':2:' but got: " << message;
    }
}

TEST(Desalter, MissingFileThrows) {
    EXPECT_THROW(load_salt_patterns("/no/such/file.smarts"), PatternFileError);
}

TEST(Desalter, StripsWholeFragmentCounterion) {
    // Aspirin . HCl -> aspirin survives, Cl stripped as "Halides".
    const auto result = MakeMiniDesalter().Desalt(MolFromSmiles("CC(=O)Oc1ccccc1C(=O)O.Cl"));
    EXPECT_EQ(CanonicalSmiles(result.mol), CanonicalSmiles(MolFromSmiles("CC(=O)Oc1ccccc1C(=O)O")));
    ASSERT_EQ(result.stripped_names.size(), 1u);
    EXPECT_EQ(result.stripped_names[0], "Halides");
}

TEST(Desalter, DoesNotStripCovalentHalogen) {
    // Linchpin safety test: a mono-fragment aryl chloride is untouched — the
    // [F,Cl,Br,I] pattern must NOT delete a covalently-bound Cl, because it is
    // not a whole component.
    const std::string input = "Clc1ccccc1C(=O)O";
    const auto result = MakeMiniDesalter().Desalt(MolFromSmiles(input));
    EXPECT_EQ(CanonicalSmiles(result.mol), CanonicalSmiles(MolFromSmiles(input)));
    EXPECT_TRUE(result.stripped_names.empty());
}

TEST(Desalter, ChargeAgnosticStripsSodiumCation) {
    const auto result = MakeMiniDesalter().Desalt(MolFromSmiles("CC(=O)[O-].[Na+]"));
    EXPECT_TRUE(result.mol.NumAtoms() == 0 || CanonicalSmiles(result.mol).find("Na") == std::string::npos);
    EXPECT_FALSE(result.stripped_names.empty());
}

TEST(Desalter, StripsMultipleCounterions) {
    const auto result = MakeMiniDesalter().Desalt(MolFromSmiles("c1ccncc1C(=O)NC.Cl.[Na]"));
    EXPECT_EQ(result.stripped_names.size(), 2u);
    EXPECT_GT(result.mol.NumAtoms(), 0u);
}

TEST(Desalter, KeepsAllNonSaltFragments) {
    const std::string input = "c1ccccc1CCN.c1ccccc1CCO";
    const auto result = MakeMiniDesalter().Desalt(MolFromSmiles(input));
    EXPECT_NE(CanonicalSmiles(result.mol).find('.'), std::string::npos);
    EXPECT_TRUE(result.stripped_names.empty());
}

TEST(Desalter, AllSaltYieldsEmptyMolecule) {
    const auto result = MakeMiniDesalter().Desalt(MolFromSmiles("[Na].Cl"));
    EXPECT_EQ(result.mol.NumAtoms(), 0u);
    EXPECT_EQ(result.stripped_names.size(), 2u);
}

TEST(Desalter, SingleComponentSaltFormerSurvivesByDefault) {
    const std::string input = "OC=O";  // formic acid, single component
    const auto result = MakeMiniDesalter().Desalt(MolFromSmiles(input));
    EXPECT_EQ(CanonicalSmiles(result.mol), CanonicalSmiles(MolFromSmiles(input)));
    EXPECT_TRUE(result.stripped_names.empty());
}

TEST(Desalter, AggressiveStripsSingleComponentSaltFormer) {
    const auto desalter = Desalter(
        load_salt_patterns(WriteTempSaltFile(kMiniSalts)),
        /*aggressive=*/true
    );
    const auto result = desalter.Desalt(MolFromSmiles("OC=O"));
    EXPECT_EQ(result.mol.NumAtoms(), 0u);
    ASSERT_EQ(result.stripped_names.size(), 1u);
    EXPECT_EQ(result.stripped_names[0], "Carboxylate counterion");
}

TEST(Desalter, NonAggressiveStillDesaltsMultiComponentInputs) {
    const auto result = MakeMiniDesalter().Desalt(MolFromSmiles("CC(=O)Oc1ccccc1C(=O)O.Cl"));
    EXPECT_EQ(CanonicalSmiles(result.mol), CanonicalSmiles(MolFromSmiles("CC(=O)Oc1ccccc1C(=O)O")));
    ASSERT_EQ(result.stripped_names.size(), 1u);
    EXPECT_EQ(result.stripped_names[0], "Halides");
}

TEST(Desalter, IsAggressiveReflectsConstruction) {
    EXPECT_FALSE(MakeMiniDesalter().IsAggressive());
    const auto aggressive = Desalter(
        load_salt_patterns(WriteTempSaltFile(kMiniSalts)),
        /*aggressive=*/true
    );
    EXPECT_TRUE(aggressive.IsAggressive());
}

TEST(Desalter, SolventsStrippedOnlyWhenSolventFileLoaded) {
    const std::string salt_path = WriteTempSaltFile(kMiniSalts);
    const std::string solvent_path = std::string(::testing::TempDir()) + "/oedesalt_test_solvents.smarts";
    { std::ofstream out(solvent_path); out << kMiniSolvents; }

    const std::string input = "c1ccncc1C(=O)NC.O";  // drug . water
    const auto salts_only = Desalter::FromFiles(salt_path).Desalt(MolFromSmiles(input));
    EXPECT_NE(CanonicalSmiles(salts_only.mol).find('.'), std::string::npos);
    const auto with_solvents = Desalter::FromFiles(salt_path, solvent_path).Desalt(MolFromSmiles(input));
    EXPECT_EQ(CanonicalSmiles(with_solvents.mol).find('.'), std::string::npos);
}

TEST(Desalter, PreservesTitleAcrossDesalting) {
    const auto desalter = MakeMiniDesalter();

    OEChem::OEGraphMol aspirin_hcl = MolFromSmiles("CC(=O)Oc1ccccc1C(=O)O.Cl");
    aspirin_hcl.SetTitle("aspirin_hcl");
    const auto result1 = desalter.Desalt(aspirin_hcl);
    EXPECT_EQ(result1.mol.GetTitle(), std::string("aspirin_hcl"));

    OEChem::OEGraphMol cocrystal = MolFromSmiles("c1ccccc1CCN.c1ccccc1CCO");
    cocrystal.SetTitle("cocrystal");
    const auto result2 = desalter.Desalt(cocrystal);
    EXPECT_EQ(result2.mol.GetTitle(), std::string("cocrystal"));

    OEChem::OEGraphMol all_salt = MolFromSmiles("[Na].Cl");
    all_salt.SetTitle("salt_only");
    const auto result3 = desalter.Desalt(all_salt);
    EXPECT_EQ(result3.mol.GetTitle(), std::string("salt_only"));
}

}  // namespace test
}  // namespace OEDESALT
