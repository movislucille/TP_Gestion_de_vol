# TP_Gestion_de_vol
Une agence de voyage située à Cotonou souhaite informatiser la gestion de ses vols et de ses réservations de billets d'avion afin de remplacer les méthodes manuelles sources d'erreurs (doublons de réservations, méconnaissance des places disponibles, suivi des paiements complexe et lenteur dans la recherche de passagers).


L'application doit répondre aux règles de gestion suivantes :

1. Un passager doit être enregistré avant de pouvoir réserver un billet.
2. Un vol ne peut pas recevoir plus de réservations que sa capacité maximale.
3. Un passager ne peut pas réserver deux fois le même vol (unicité active).
4. Une réservation est associée à un seul passager et un seul vol.
5. Une réservation annulée ne peut plus être payée.
6. Une place est définitivement réservée uniquement après validation de la réservation.
7. Une réservation payée et annulée fait l'objet d'un traitement de remboursement.
