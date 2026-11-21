#include <cstdlib>
#include "framework.h"
// <Arduino.h> y no "shim/Arduino.h": este main lo comparten DOS
// binarios. El de siempre usa test/shim; el de la interfaz
// (tamapoke_uitests) usa los shims del simulador, porque el sketch
// necesita pantalla, tactil y Wire. Quien elige es la ruta de
// inclusion del Makefile, no este fichero.
#include <Arduino.h>
#include <cstdio>
#include <cstring>

std::vector<TestCase> &testRegistry() {
  static std::vector<TestCase> reg;
  return reg;
}

int gAssertCount = 0;
static std::vector<std::string> gFailures;

void testFail(const char *file, int line, const std::string &msg) {
  const char *base = strrchr(file, '/');
  std::ostringstream os;
  os << (base ? base + 1 : file) << ":" << line << ": " << msg;
  gFailures.push_back(os.str());
}

int main(int argc, char **argv) {
  const char *filter = argc > 1 ? argv[1] : 0;
  // Sin bufer: si un test se lleva el proceso por delante (una
  // violacion de acceso no da ni la oportunidad de vaciar), lo ultimo
  // que se ve en pantalla es el test que estaba corriendo. Con bufer
  // se pierde TODO y uno se queda con un codigo de salida y nada mas,
  // que es como me pase media hora buscando a ciegas.
  setvbuf(stdout, nullptr, _IONBF, 0);
  mockSerialSilence(true);

  int passed = 0, failed = 0, skipped = 0, xfail = 0, xpass = 0;
  std::string lastSuite;
  std::vector<std::string> failedNames;
  std::vector<std::string> knownIssues;
  std::vector<std::string> fixedIssues;

  for (size_t i = 0; i < testRegistry().size(); i++) {
    TestCase &tc = testRegistry()[i];
    std::string full = tc.suite + "." + tc.name;
    if (filter && full.find(filter) == std::string::npos) { skipped++; continue; }
    // Para bisectar um teste que so falha DEPOIS de outros: TP_DESDE/TP_ATE
    // rodam so a faixa [desde, ate] da ordem de registro; TP_LISTA=1 imprime
    // o indice de cada um.
    {
      const char *d = getenv("TP_DESDE"), *t = getenv("TP_ATE");
      if ((d && (long)i < atol(d)) || (t && (long)i > atol(t))) { skipped++; continue; }
      if (getenv("TP_LISTA")) printf("  #%u %s" "\n", (unsigned)i, full.c_str());
    }
    if (tc.suite != lastSuite) {
      printf("\n\033[1m%s\033[0m\n", tc.suite.c_str());
      lastSuite = tc.suite;
    }
    gFailures.clear();
    // estado limpio para cada test
    mockSetMillis(0);
    mockClearForcedRandom();
    randomSeed(0xC0FFEE);
    tc.fn();
    if (tc.knownIssue) {
      if (gFailures.empty()) {
        printf("  \033[33mXPASS\033[0m %s  (ya no falla: quita TEST_KNOWN_ISSUE)\n", tc.name.c_str());
        xpass++;
        fixedIssues.push_back(full);
      } else {
        printf("  \033[33mKNOWN\033[0m %s  -- %s\n", tc.name.c_str(), tc.note.c_str());
        for (size_t j = 0; j < gFailures.size(); j++)
          printf("        %s\n", gFailures[j].c_str());
        xfail++;
        knownIssues.push_back(full + ": " + tc.note);
      }
      continue;
    }
    if (gFailures.empty()) {
      printf("  \033[32mok\033[0m   %s\n", tc.name.c_str());
      passed++;
    } else {
      printf("  \033[31mFAIL\033[0m %s\n", tc.name.c_str());
      for (size_t j = 0; j < gFailures.size(); j++)
        printf("        %s\n", gFailures[j].c_str());
      failed++;
      failedNames.push_back(full);
    }
  }

  printf("\n----------------------------------------\n");
  printf("%d tests, %d asserts, %d passed, %d failed", passed + failed + xfail + xpass,
         gAssertCount, passed, failed);
  if (xfail) printf(", %d bugs conocidos", xfail);
  if (xpass) printf(", %d bugs ya arreglados", xpass);
  if (skipped) printf(", %d filtrados", skipped);
  printf("\n");
  if (failed) {
    printf("\nFallos:\n");
    for (size_t i = 0; i < failedNames.size(); i++) printf("  - %s\n", failedNames[i].c_str());
  }
  if (xfail) {
    printf("\nBugs conocidos (documentados, no rompen la suite):\n");
    for (size_t i = 0; i < knownIssues.size(); i++) printf("  - %s\n", knownIssues[i].c_str());
  }
  if (xpass) {
    printf("\nBugs conocidos que ya NO fallan (quita TEST_KNOWN_ISSUE):\n");
    for (size_t i = 0; i < fixedIssues.size(); i++) printf("  - %s\n", fixedIssues[i].c_str());
  }
  return failed ? 1 : 0;
}
