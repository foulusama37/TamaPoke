# -*- coding: utf-8 -*-
"""Las 121 cadenas de la interfaz en portugues de BRASIL.

Estaban en portugues de PORTUGAL, y el dueno lo corto en seco: "nada de
portugues de portugal... e pt-br". No era solo acento: el boton de sexo
decia RAPARIGA, que en Brasil es un insulto, y los textos del profesor
hablaban de tu ("Da-lhe de comer", "Cuida bem dela e ela luta por ti").

Lo que cambia, aparte de los acentos:
  tu  -> voce            "Cuide bem dela e ela luta por voce"
  rapaz/rapariga -> menino/menina
  "Escolhe"/"Carrega" -> "Escolha"/"Copie"   (imperativo de voce)
  "quer dizer-te"  -> "quer te dizer"        (pronombre delante)
  "Pronto a evoluir" -> "Pronto para evoluir"

Las MAYUSCULAS que CP437 no tiene
---------------------------------
De las acentuadas solo hay E (\\220) y C-cedilla (\\200); A~ y O~ los pinta
printT() aparte. Las demas NO existen, y eso decide como se escriben
algunas etiquetas. No se deja ninguna sin acento "porque casi no se nota":

  SEM FOLEGO   -> "SEM FORCA"      (cedilla si existe)
  OTIMO!       -> "MUITO BEM!"
  TREINAR O QUE? -> "O QUE TREINAR?"  (ahi "que" no lleva acento)
  IR AO GINASIO  -> "Ir ao Ginasio"   (en minuscula el acento si cabe)
  GINASIOS       -> "Ginasios"
  PREMIO: %s     -> "Premio: %s"
  EVOLUCAO, CAMPEAO, NAO  -> se quedan en mayuscula: solo necesitan A~

Los %s/%u/%d no se tocan: test_i18n.cpp compara la firma de cada cadena
contra las otras siete traducciones y salta si una pierde un formato.
"""

# El orden es el del enum StrId de i18n.h. Si se anade una cadena nueva hay
# que anadirla AQUI tambien, en su sitio.
UI = [
    # estado del bicho
    "Evoluindo!", "Nham nham!", "Ele gosta!", "Está com fome!",
    "Precisa de banho!",
    "Exausto...", "Triste...", "Um pouco gordinho...", "É SHINY!!",
    "Está feliz",
    # despedidas
    "OBRIGADO! Até sempre", "Fugiu...", "Tchau! Se despede...",
    # huevo
    "OVO", "Ovo lendário!?", "Ovo raro!", "Toque no ovo...", "Mexeu!",
    "Quase lá!",
    # formatos compartidos
    "POKEDEX %u/%u",
    "%s%s Nv.%u",
    # soltar
    "Soltar %s?", "SIM", "NÃO",
    # gimnasio: saco de tipos
    "O QUE TREINAR?", "NOVO RECORDE!", "RECORDE: %u", "ACERTE A FRAQUEZA!",
    "PONTOS: %u", "SUPER EFICAZ!", "ACERTOU", "NEM SENTIU",
    "EV %u/%u", "%s +%u", "NO LIMITE", "SEM FORÇA",
    # reloj / ajustes
    "AJUSTAR HORA", "HORA", "MIN", "deslize p/ cima: cancelar", "Idioma",
    # celebracion
    "MEDALHA!", "MUITO BEM!", "%u DIAS SEGUIDOS!",
    # ficha: perfil
    "SEQ %u  rec %u", "LAÇO", "BAGA ???", "BAGA VERMELHA", "BAGA AZUL",
    "BAGA VERDE",
    "%s   IDADE %lud", "toque no nome: renomear", "DESOBEDECE",
    # ficha: combate
    "COMBATE", "ATQ", "DEF", "AT.E", "DF.E", "VEL", "PES", "Ir ao Ginásio",
    # ficha: medalhas
    "MEDALHAS %d/%d", "toque: voltar",
    # teclado y galeria
    "NOME:", "toque para voltar",
    # barras
    "COM", "ALE", "ENE", "HIG",
    # marcador del gimnasio
    "REC %u",
    # ficha: progreso
    "PROGRESSO", "Nv.%u", "%u min para Nv.%u", "EVOLUÇÃO", "Forma final",
    "Pronto para evoluir!", "Tudo a 40 para evoluir",
    "Evolui em %u nv.", "Descuidos: %u",
    # sonido
    "SOM ON", "SOM OFF",
    # gimnasios de Kanto
    "Ginásios", "%u/%u vitórias", "DESAFIAR", "Ainda não está pronto",
    "CAMPEÃO DE %s!",
    # intro y reseteo
    "PRECIOUS EDITION", "baseado no TamaPoke de", "toque para continuar",
    "Olá! Eu sou o professor Carvalho.",
    "Este mundo está cheio de criaturas chamadas POKEMON.",
    "Uma delas vai viver com você. Dê comida, brinque e deixe dormir.",
    "Cuide bem dela e ela luta por você. Descuide e ela para de obedecer.",
    "Você é menino ou menina?", "MENINO", "MENINA",
    "APAGAR TUDO", "Apagar o jogo?", "Tem certeza? Não tem volta.",
    "EVOLUIR",
    # ceremonias
    "%s quer te dizer algo...", "%s se sente abandonado...",
    "Evoluir?", "Manter forma", "Despedir?", "Adeus", "Ficar juntos",
    # inicial y sprites
    "Escolha o inicial", "Sem sprites", "Copie para o SD",
    # quiz
    "HORA DO QUIZ!", "De que tipo é %s?", "%s evolui em?",
    "De que tipo é o golpe %s?", "Qual pesa mais?", "Que golpe %s aprende?",
    "CERTO!", "ERROU", "%u de %u", "Prêmio: %s", "Fica para a próxima!",
    # progreso (estaban en ingles en los ocho idiomas)
    "TETO Nv.%u", "TETO %u", "%lu EXP para Nv.%u",
    # titulos de la ficha (tambien estaban en ingles)
    "GOLPES", "MOCHILA", "EQUIPE", "CAIXA",
]
