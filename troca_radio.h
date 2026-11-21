#pragma once
// O radio das trocas. Na placa: ESP-NOW (Wi-Fi em STA, canal 1), ligado so
// com a tela de troca aberta — o Wi-Fi come uns 100 mA e ~50 KB de RAM, e
// fora dela nao serve para nada. No PC (simulador e testes de tela): uma
// "placa fantasma" que anuncia, aceita o convite e oferece um Kadabra, para
// a tela poder ser exercitada sem duas placas.
#include <stdint.h>

bool radioLiga();
void radioDesliga();
bool radioLigado();
void radioMac(uint8_t mac[6]);
void radioEnvia(const uint8_t *mac, const uint8_t *buf, int n);
// o proximo pacote que chegou (copiado para buf); 0 = nenhum
int radioRecebe(uint8_t *buf, int cap);
// PC: o relogio da fantasma anda junto com a tela
void radioTick(uint32_t agora);
// So no PC (testes): a fantasma some, ou passa a ver outro combate.
void radioFantasmaCala(bool s);
void radioFantasmaHashErrado(bool s);
void radioFantasmaConvida(uint8_t regras);
