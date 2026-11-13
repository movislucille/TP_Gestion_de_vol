// ============================================================================
//  Gestion des vols et des réservations - Agence de voyage de Cotonou
//  Fichier unique : toutes les classes + le menu console.
//  Compilation :  g++ -std=c++11 -Wall -Wextra -o agence agence_voyage.cpp
//  (compatible C++11, C++14, C++17 et Visual Studio)
// ============================================================================

#define _CRT_SECURE_NO_WARNINGS   // évite l'erreur C4996 (localtime) sous Visual Studio

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// ============================================================================
//  OUTILS
// ============================================================================

// Supprime les espaces au début et à la fin d'une chaîne.
string trim(const string& s) {
    size_t debut = s.find_first_not_of(" \t\r\n");
    if (debut == string::npos) return "";
    size_t fin = s.find_last_not_of(" \t\r\n");
    return s.substr(debut, fin - debut + 1);
}

// Convertit en majuscules (comparaison des numéros de passeport / de vol).
string majuscules(string s) {
    for (auto& c : s) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return s;
}

// Date du jour au format JJ/MM/AAAA.
string dateDuJour() {
    time_t maintenant = time(nullptr);
    tm* t = localtime(&maintenant);
    char tampon[16];
    strftime(tampon, sizeof(tampon), "%d/%m/%Y", t);
    return tampon;
}

// Formate un montant sans décimales (ex. 50000 -> "50000 FCFA").
string formaterMontant(double montant) {
    ostringstream flux;
    flux << fixed << setprecision(0) << montant << " FCFA";
    return flux.str();
}

string champObligatoire(const string& valeur, const string& libelle) {
    string v = trim(valeur);
    if (v.empty()) throw invalid_argument(libelle + " ne peut pas être vide.");
    return v;
}

bool tousChiffres(const string& s, size_t debut, size_t fin) {
    for (size_t i = debut; i < fin; ++i) {
        if (!isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

// ============================================================================
//  ÉNUMÉRATIONS
// ============================================================================

// Règle 6 : la place n'est définitive qu'une fois la réservation CONFIRMEE.
enum class StatutReservation {
    EN_ATTENTE,   // créée, place bloquée provisoirement, paiement attendu
    CONFIRMEE,    // payée, place définitivement réservée
    ANNULEE       // annulée, place libérée
};

// Règle 7 : gestion du remboursement.
enum class StatutPaiement {
    NON_PAYE,       // aucun paiement enregistré
    PAYE,           // paiement reçu
    A_REMBOURSER,   // réservation payée puis annulée : remboursement à effectuer
    REMBOURSE       // remboursement effectué
};

string versTexte(StatutReservation s) {
    switch (s) {
        case StatutReservation::EN_ATTENTE: return "EN_ATTENTE";
        case StatutReservation::CONFIRMEE:  return "CONFIRMEE";
        case StatutReservation::ANNULEE:    return "ANNULEE";
    }
    return "INCONNU";
}

string versTexte(StatutPaiement s) {
    switch (s) {
        case StatutPaiement::NON_PAYE:     return "NON_PAYE";
        case StatutPaiement::PAYE:         return "PAYE";
        case StatutPaiement::A_REMBOURSER: return "A_REMBOURSER";
        case StatutPaiement::REMBOURSE:    return "REMBOURSE";
    }
    return "INCONNU";
}

// ============================================================================
//  CLASSE Passager
// ============================================================================

class Passager {
private:
    string nom;
    string prenom;
    string telephone;
    string numeroPasseport;

    static string telephoneValide(const string& valeur) {
        string v = trim(valeur);
        int chiffres = 0;
        for (char c : v) {
            if (isdigit(static_cast<unsigned char>(c))) {
                ++chiffres;
            } else if (c != '+' && c != ' ' && c != '-') {
                throw invalid_argument("Le téléphone ne doit contenir que des chiffres, '+', espaces ou '-'.");
            }
        }
        if (chiffres < 8) throw invalid_argument("Le téléphone doit contenir au moins 8 chiffres.");
        return v;
    }

    static string passeportValide(const string& valeur) {
        string v = majuscules(trim(valeur));
        if (v.size() < 5 || v.size() > 15) {
            throw invalid_argument("Le numéro de passeport doit contenir entre 5 et 15 caractères.");
        }
        for (char c : v) {
            if (!isalnum(static_cast<unsigned char>(c))) {
                throw invalid_argument("Le numéro de passeport ne doit contenir que des lettres et des chiffres.");
            }
        }
        return v;
    }

public:
    Passager(const string& nom, const string& prenom,
             const string& telephone, const string& numeroPasseport)
        : nom(champObligatoire(nom, "Le nom")),
          prenom(champObligatoire(prenom, "Le prénom")),
          telephone(telephoneValide(telephone)),
          numeroPasseport(passeportValide(numeroPasseport)) {}

    const string& getNom() const { return nom; }
    const string& getPrenom() const { return prenom; }
    const string& getTelephone() const { return telephone; }
    const string& getNumeroPasseport() const { return numeroPasseport; }

    void setTelephone(const string& nouveauTelephone) {
        telephone = telephoneValide(nouveauTelephone);
    }

    string nomComplet() const { return prenom + " " + nom; }

    void afficher() const {
        cout << "Passeport " << numeroPasseport << " | " << nomComplet()
             << " | Tél : " << telephone << '\n';
    }
};

// ============================================================================
//  CLASSE Vol
// ============================================================================

class Vol {
private:
    string numero;
    string villeDepart;
    string villeArrivee;
    string date;      // JJ/MM/AAAA
    string heure;     // HH:MM
    double prix;
    int capacite;
    int placesOccupees;

    static string dateValide(const string& valeur) {
        string v = trim(valeur);
        if (v.size() != 10 || v[2] != '/' || v[5] != '/' ||
            !tousChiffres(v, 0, 2) || !tousChiffres(v, 3, 5) || !tousChiffres(v, 6, 10)) {
            throw invalid_argument("Date invalide (format attendu : JJ/MM/AAAA).");
        }
        int jour = stoi(v.substr(0, 2));
        int mois = stoi(v.substr(3, 2));
        if (jour < 1 || jour > 31 || mois < 1 || mois > 12) {
            throw invalid_argument("Date invalide : jour ou mois hors limites.");
        }
        return v;
    }

    static string heureValide(const string& valeur) {
        string v = trim(valeur);
        if (v.size() != 5 || v[2] != ':' || !tousChiffres(v, 0, 2) || !tousChiffres(v, 3, 5)) {
            throw invalid_argument("Heure invalide (format attendu : HH:MM).");
        }
        int h = stoi(v.substr(0, 2));
        int m = stoi(v.substr(3, 2));
        if (h > 23 || m > 59) throw invalid_argument("Heure invalide : valeurs hors limites.");
        return v;
    }

public:
    Vol(const string& numero, const string& villeDepart, const string& villeArrivee,
        const string& date, const string& heure, double prix, int capacite)
        : numero(majuscules(champObligatoire(numero, "Le numéro de vol"))),
          villeDepart(champObligatoire(villeDepart, "La ville de départ")),
          villeArrivee(champObligatoire(villeArrivee, "La ville d'arrivée")),
          date(dateValide(date)),
          heure(heureValide(heure)),
          prix(prix),
          capacite(capacite),
          placesOccupees(0) {
        if (majuscules(this->villeDepart) == majuscules(this->villeArrivee)) {
            throw invalid_argument("La ville de départ et la ville d'arrivée doivent être différentes.");
        }
        if (prix <= 0) throw invalid_argument("Le prix du billet doit être strictement positif.");
        if (capacite <= 0) throw invalid_argument("Le nombre de places doit être strictement positif.");
    }

    const string& getNumero() const { return numero; }
    const string& getVilleDepart() const { return villeDepart; }
    const string& getVilleArrivee() const { return villeArrivee; }
    const string& getDate() const { return date; }
    const string& getHeure() const { return heure; }
    double getPrix() const { return prix; }
    int getCapacite() const { return capacite; }
    int getPlacesOccupees() const { return placesOccupees; }

    int getPlacesRestantes() const { return capacite - placesOccupees; }
    bool estComplet() const { return placesOccupees >= capacite; }

    // Règle 2 : un vol ne peut pas recevoir plus de réservations que sa capacité.
    void occuperPlace() {
        if (estComplet()) throw logic_error("Le vol " + numero + " est complet.");
        ++placesOccupees;
    }

    void libererPlace() {
        if (placesOccupees <= 0) throw logic_error("Aucune place à libérer sur le vol " + numero + ".");
        --placesOccupees;
    }

    void afficher() const {
        cout << numero << " | " << villeDepart << " -> " << villeArrivee
             << " | " << date << " à " << heure
             << " | " << formaterMontant(prix)
             << " | places restantes : " << getPlacesRestantes() << "/" << capacite << '\n';
    }
};

// ============================================================================
//  CLASSE Paiement
// ============================================================================

class Paiement {
private:
    double montant;
    string date;
    StatutPaiement statut;

public:
    // Un paiement nouvellement créé a le statut PAYE.
    Paiement(double montant, const string& date)
        : montant(montant), date(date), statut(StatutPaiement::PAYE) {
        if (montant <= 0) throw invalid_argument("Le montant du paiement doit être strictement positif.");
    }

    double getMontant() const { return montant; }
    const string& getDate() const { return date; }
    StatutPaiement getStatut() const { return statut; }

    // PAYE -> A_REMBOURSER
    void marquerARembourser() {
        if (statut != StatutPaiement::PAYE) {
            throw logic_error("Seul un paiement au statut PAYE peut passer à A_REMBOURSER.");
        }
        statut = StatutPaiement::A_REMBOURSER;
    }

    // A_REMBOURSER -> REMBOURSE
    void marquerRembourse() {
        if (statut != StatutPaiement::A_REMBOURSER) {
            throw logic_error("Le paiement n'est pas en attente de remboursement.");
        }
        statut = StatutPaiement::REMBOURSE;
    }

    void afficher() const {
        cout << formaterMontant(montant) << " le " << date
             << " (" << versTexte(statut) << ")\n";
    }
};

// ============================================================================
//  CLASSE Reservation
// ============================================================================

class Reservation {
private:
    string numero;
    shared_ptr<Passager> passager;   // association : 1 seul passager (règle 4)
    shared_ptr<Vol> vol;             // association : 1 seul vol (règle 4)
    StatutReservation statut;
    shared_ptr<Paiement> paiement;   // composition : 0..1 paiement (nullptr = pas de paiement)

public:
    Reservation(const string& numero, shared_ptr<Passager> passager, shared_ptr<Vol> vol)
        : numero(numero),
          passager(std::move(passager)),
          vol(std::move(vol)),
          statut(StatutReservation::EN_ATTENTE) {
        if (!this->passager) throw invalid_argument("Une réservation doit être associée à un passager.");
        if (!this->vol) throw invalid_argument("Une réservation doit être associée à un vol.");
    }

    const string& getNumero() const { return numero; }
    const shared_ptr<Passager>& getPassager() const { return passager; }
    const shared_ptr<Vol>& getVol() const { return vol; }
    StatutReservation getStatut() const { return statut; }
    const shared_ptr<Paiement>& getPaiement() const { return paiement; }

    bool estPayee() const {
        return paiement != nullptr && paiement->getStatut() == StatutPaiement::PAYE;
    }

    StatutPaiement getStatutPaiement() const {
        return paiement ? paiement->getStatut() : StatutPaiement::NON_PAYE;
    }

    // Règle 5 : interdit si la réservation est annulée (ou déjà payée).
    // Règle 6 : la réservation passe à CONFIRMEE.
    void enregistrerPaiement(const string& date) {
        if (statut == StatutReservation::ANNULEE) {
            throw logic_error("La réservation " + numero + " est annulée : paiement impossible.");
        }
        if (paiement != nullptr) {
            throw logic_error("La réservation " + numero + " a déjà été payée.");
        }
        paiement = make_shared<Paiement>(vol->getPrix(), date);
        statut = StatutReservation::CONFIRMEE;
    }

    // Règle 7 : si le paiement existe, il passe à A_REMBOURSER.
    void annuler() {
        if (statut == StatutReservation::ANNULEE) {
            throw logic_error("La réservation " + numero + " est déjà annulée.");
        }
        statut = StatutReservation::ANNULEE;
        if (paiement != nullptr && paiement->getStatut() == StatutPaiement::PAYE) {
            paiement->marquerARembourser();
        }
    }

    // Marque le remboursement comme effectué (réservation annulée et payée).
    void rembourser() {
        if (statut != StatutReservation::ANNULEE) {
            throw logic_error("Seule une réservation annulée peut être remboursée.");
        }
        if (paiement == nullptr) {
            throw logic_error("La réservation " + numero + " n'a pas été payée : rien à rembourser.");
        }
        paiement->marquerRembourse();
    }

    void afficher() const {
        cout << numero << " | " << passager->nomComplet()
             << " | Vol " << vol->getNumero() << " (" << vol->getVilleDepart()
             << " -> " << vol->getVilleArrivee() << ")"
             << " | " << versTexte(statut)
             << " | paiement : " << versTexte(getStatutPaiement()) << '\n';
    }

    void afficherDetails() const {
        cout << "------------------------------------------\n"
             << "Réservation        : " << numero << '\n'
             << "Passager           : " << passager->nomComplet()
             << " (passeport " << passager->getNumeroPasseport() << ")\n"
             << "Téléphone          : " << passager->getTelephone() << '\n'
             << "Vol                : " << vol->getNumero() << " "
             << vol->getVilleDepart() << " -> " << vol->getVilleArrivee() << '\n'
             << "Départ             : " << vol->getDate() << " à " << vol->getHeure() << '\n'
             << "Prix du billet     : " << formaterMontant(vol->getPrix()) << '\n'
             << "Statut réservation : " << versTexte(statut) << '\n'
             << "Statut paiement    : " << versTexte(getStatutPaiement());
        if (paiement != nullptr) {
            cout << " (" << formaterMontant(paiement->getMontant())
                 << " le " << paiement->getDate() << ")";
        }
        cout << "\n------------------------------------------\n";
    }
};

// ============================================================================
//  CLASSE Agence
//  Stocke les données dans des vector et applique les règles de gestion.
//  Erreurs : invalid_argument (donnée invalide), runtime_error (introuvable),
//            logic_error (opération interdite).
// ============================================================================

class Agence {
private:
    string nom;
    vector<shared_ptr<Passager>> passagers;
    vector<shared_ptr<Vol>> vols;
    vector<Reservation> reservations;
    int compteurReservations;

    string genererNumeroReservation() {
        ostringstream flux;
        flux << "RES-" << setw(3) << setfill('0') << ++compteurReservations;
        return flux.str();
    }

    Reservation& trouverReservation(const string& numero) {
        string cle = majuscules(trim(numero));
        for (auto& r : reservations) {
            if (r.getNumero() == cle) return r;
        }
        throw runtime_error("Réservation introuvable : " + cle);
    }

public:
    explicit Agence(const string& nom) : nom(nom), compteurReservations(0) {}

    const string& getNom() const { return nom; }

    // ------------------------------------------------------------ Passagers
    shared_ptr<Passager> rechercherPassager(const string& passeport) const {
        string cle = majuscules(trim(passeport));
        for (const auto& p : passagers) {
            if (p->getNumeroPasseport() == cle) return p;
        }
        return nullptr;
    }

    void enregistrerPassager(const string& nom, const string& prenom,
                             const string& telephone, const string& passeport) {
        auto nouveau = make_shared<Passager>(nom, prenom, telephone, passeport);
        if (rechercherPassager(nouveau->getNumeroPasseport())) {
            throw logic_error("Un passager avec le passeport " +
                              nouveau->getNumeroPasseport() + " est déjà enregistré.");
        }
        passagers.push_back(nouveau);
    }

    const vector<shared_ptr<Passager>>& getPassagers() const { return passagers; }

    // ----------------------------------------------------------------- Vols
    shared_ptr<Vol> rechercherVol(const string& numero) const {
        string cle = majuscules(trim(numero));
        for (const auto& v : vols) {
            if (v->getNumero() == cle) return v;
        }
        return nullptr;
    }

    void ajouterVol(const string& numero, const string& depart, const string& arrivee,
                    const string& date, const string& heure, double prix, int capacite) {
        auto nouveau = make_shared<Vol>(numero, depart, arrivee, date, heure, prix, capacite);
        if (rechercherVol(nouveau->getNumero())) {
            throw logic_error("Le vol " + nouveau->getNumero() + " existe déjà.");
        }
        vols.push_back(nouveau);
    }

    vector<shared_ptr<Vol>> volsDisponibles() const {
        vector<shared_ptr<Vol>> resultat;
        for (const auto& v : vols) {
            if (!v->estComplet()) resultat.push_back(v);
        }
        return resultat;
    }

    // --------------------------------------------------------- Réservations
    // Retourne le numéro de la réservation créée (règles 1, 2, 3, 4).
    string reserver(const string& passeport, const string& numeroVol) {
        // Règle 1 : le passager doit être enregistré.
        auto passager = rechercherPassager(passeport);
        if (!passager) {
            throw runtime_error("Passager non enregistré (passeport " +
                                majuscules(trim(passeport)) + "). Enregistrez-le d'abord.");
        }

        auto vol = rechercherVol(numeroVol);
        if (!vol) throw runtime_error("Vol introuvable : " + majuscules(trim(numeroVol)));

        // Règle 3 : pas de seconde réservation (non annulée) du même vol par le même passager.
        for (const auto& r : reservations) {
            if (r.getStatut() != StatutReservation::ANNULEE &&
                r.getPassager() == passager && r.getVol() == vol) {
                throw logic_error("Le passager " + passager->nomComplet() +
                                  " a déjà réservé le vol " + vol->getNumero() +
                                  " (" + r.getNumero() + ").");
            }
        }

        // Règle 2 : occuperPlace() lève une exception si le vol est complet.
        vol->occuperPlace();

        // Règle 6 : la réservation démarre EN_ATTENTE (place bloquée provisoirement).
        reservations.emplace_back(genererNumeroReservation(), passager, vol);
        return reservations.back().getNumero();
    }

    const Reservation& obtenirReservation(const string& numero) const {
        string cle = majuscules(trim(numero));
        for (const auto& r : reservations) {
            if (r.getNumero() == cle) return r;
        }
        throw runtime_error("Réservation introuvable : " + cle);
    }

    vector<Reservation> reservationsDuPassager(const string& passeport) const {
        auto passager = rechercherPassager(passeport);
        if (!passager) {
            throw runtime_error("Passager non enregistré (passeport " +
                                majuscules(trim(passeport)) + ").");
        }
        vector<Reservation> resultat;
        for (const auto& r : reservations) {
            if (r.getPassager() == passager) resultat.push_back(r);
        }
        return resultat;
    }

    const vector<Reservation>& getReservations() const { return reservations; }

    // ------------------------------------------- Paiements et annulations
    void enregistrerPaiement(const string& numeroReservation) {
        // Règle 5 : Reservation::enregistrerPaiement refuse une réservation annulée.
        trouverReservation(numeroReservation).enregistrerPaiement(dateDuJour());
    }

    // Retourne true si la réservation était payée (un remboursement est alors dû).
    bool annulerReservation(const string& numeroReservation) {
        Reservation& r = trouverReservation(numeroReservation);
        bool etaitPayee = r.estPayee();
        r.annuler();                  // lève une exception si déjà annulée
        r.getVol()->libererPlace();   // la place redevient disponible
        return etaitPayee;            // règle 7 : remboursement à traiter si payée
    }

    void rembourser(const string& numeroReservation) {
        trouverReservation(numeroReservation).rembourser();
    }
};

// ============================================================================
//  SAISIE ET AFFICHAGE (menu console)
// ============================================================================

// Lit une ligne ; arrête proprement le programme si l'entrée est terminée.
string lireTexte(const string& invite) {
    cout << invite;
    string ligne;
    if (!getline(cin, ligne)) {
        cout << "\n[Fin de l'entrée : arrêt du programme]\n";
        exit(0);
    }
    return trim(ligne);
}

int lireEntier(const string& invite) {
    string texte = lireTexte(invite);
    size_t position = 0;
    int valeur = 0;
    try {
        valeur = stoi(texte, &position);
    } catch (const exception&) {
        throw invalid_argument("Entier invalide : \"" + texte + "\".");
    }
    if (position != texte.size()) throw invalid_argument("Entier invalide : \"" + texte + "\".");
    return valeur;
}

double lireReel(const string& invite) {
    string texte = lireTexte(invite);
    for (auto& c : texte) {
        if (c == ',') c = '.';
    }
    size_t position = 0;
    double valeur = 0;
    try {
        valeur = stod(texte, &position);
    } catch (const exception&) {
        throw invalid_argument("Nombre invalide : \"" + texte + "\".");
    }
    if (position != texte.size()) throw invalid_argument("Nombre invalide : \"" + texte + "\".");
    return valeur;
}

void afficherMenu() {
    cout << "\n========== AGENCE DE VOYAGE - COTONOU ==========\n"
         << " 1. Enregistrer un passager\n"
         << " 2. Ajouter un vol\n"
         << " 3. Afficher les vols disponibles\n"
         << " 4. Rechercher un vol\n"
         << " 5. Effectuer une réservation\n"
         << " 6. Enregistrer un paiement\n"
         << " 7. Annuler une réservation\n"
         << " 8. Consulter les réservations\n"
         << " 9. Afficher les informations d'une réservation\n"
         << "10. Quitter\n"
         << "11. (Bonus) Afficher la liste des passagers\n"
         << "12. (Bonus) Rechercher un passager\n"
         << "================================================\n";
}

// ============================================================================
//  ACTIONS DU MENU
// ============================================================================

void actionEnregistrerPassager(Agence& agence) {
    string nom = lireTexte("Nom : ");
    string prenom = lireTexte("Prénom : ");
    string telephone = lireTexte("Téléphone : ");
    string passeport = lireTexte("Numéro de passeport : ");
    agence.enregistrerPassager(nom, prenom, telephone, passeport);
    cout << "Passager enregistré avec succès.\n";
}

void actionAjouterVol(Agence& agence) {
    string numero = lireTexte("Numéro du vol : ");
    string depart = lireTexte("Ville de départ : ");
    string arrivee = lireTexte("Ville d'arrivée : ");
    string date = lireTexte("Date (JJ/MM/AAAA) : ");
    string heure = lireTexte("Heure (HH:MM) : ");
    double prix = lireReel("Prix du billet (FCFA) : ");
    int places = lireEntier("Nombre de places : ");
    agence.ajouterVol(numero, depart, arrivee, date, heure, prix, places);
    cout << "Vol ajouté avec succès.\n";
}

void actionVolsDisponibles(const Agence& agence) {
    cout << "--- Vols disponibles ---\n";
    auto liste = agence.volsDisponibles();
    if (liste.empty()) {
        cout << "Aucun vol à afficher.\n";
        return;
    }
    for (const auto& v : liste) v->afficher();
}

void actionRechercherVol(const Agence& agence) {
    string numero = lireTexte("Numéro du vol : ");
    auto vol = agence.rechercherVol(numero);
    if (!vol) throw runtime_error("Aucun vol avec le numéro " + majuscules(numero) + ".");
    vol->afficher();
}

void actionReserver(Agence& agence) {
    string passeport = lireTexte("Numéro de passeport du passager : ");
    string numeroVol = lireTexte("Numéro du vol : ");
    string numeroReservation = agence.reserver(passeport, numeroVol);
    cout << "Réservation " << numeroReservation << " créée (EN_ATTENTE).\n"
         << "La place est confirmée définitivement après le paiement.\n";
}

void actionPaiement(Agence& agence) {
    string numero = lireTexte("Numéro de la réservation : ");
    agence.enregistrerPaiement(numero);
    cout << "Paiement enregistré : la réservation est maintenant CONFIRMEE.\n";
}

void actionAnnuler(Agence& agence) {
    string numero = lireTexte("Numéro de la réservation à annuler : ");
    bool remboursementDu = agence.annulerReservation(numero);
    cout << "Réservation annulée, la place a été libérée.\n";
    if (remboursementDu) {
        cout << "Cette réservation était payée : paiement au statut A_REMBOURSER.\n";
        string reponse = lireTexte("Remboursement effectué maintenant ? (o/n) : ");
        if (reponse == "o" || reponse == "O") {
            agence.rembourser(numero);
            cout << "Paiement au statut REMBOURSE.\n";
        }
    }
}

void actionConsulterReservations(const Agence& agence) {
    cout << "1. Toutes les réservations\n"
         << "2. Réservations d'un passager\n";
    int choix = lireEntier("Votre choix : ");
    if (choix == 1) {
        if (agence.getReservations().empty()) {
            cout << "Aucune réservation enregistrée.\n";
            return;
        }
        for (const auto& r : agence.getReservations()) r.afficher();
    } else if (choix == 2) {
        string passeport = lireTexte("Numéro de passeport : ");
        auto liste = agence.reservationsDuPassager(passeport);
        if (liste.empty()) {
            cout << "Ce passager n'a aucune réservation.\n";
            return;
        }
        for (const auto& r : liste) r.afficher();
    } else {
        throw invalid_argument("Choix invalide (1 ou 2 attendu).");
    }
}

void actionInfosReservation(const Agence& agence) {
    string numero = lireTexte("Numéro de la réservation : ");
    agence.obtenirReservation(numero).afficherDetails();
}

void actionListePassagers(const Agence& agence) {
    cout << "--- Liste des passagers ---\n";
    if (agence.getPassagers().empty()) {
        cout << "Aucun passager enregistré.\n";
        return;
    }
    for (const auto& p : agence.getPassagers()) p->afficher();
}

void actionRechercherPassager(const Agence& agence) {
    string passeport = lireTexte("Numéro de passeport : ");
    auto passager = agence.rechercherPassager(passeport);
    if (!passager) throw runtime_error("Aucun passager avec le passeport " + majuscules(passeport) + ".");
    passager->afficher();
}

// ============================================================================
//  PROGRAMME PRINCIPAL
// ============================================================================

int main() {
    Agence agence("Agence de voyage de Cotonou");
    int choix = 0;

    do {
        afficherMenu();
        try {
            choix = lireEntier("Votre choix : ");
            switch (choix) {
                case 1:  actionEnregistrerPassager(agence); break;
                case 2:  actionAjouterVol(agence); break;
                case 3:  actionVolsDisponibles(agence); break;
                case 4:  actionRechercherVol(agence); break;
                case 5:  actionReserver(agence); break;
                case 6:  actionPaiement(agence); break;
                case 7:  actionAnnuler(agence); break;
                case 8:  actionConsulterReservations(agence); break;
                case 9:  actionInfosReservation(agence); break;
                case 10: cout << "Au revoir !\n"; break;
                case 11: actionListePassagers(agence); break;
                case 12: actionRechercherPassager(agence); break;
                default: cout << "Choix invalide : entrez un nombre entre 1 et 12.\n";
            }
        } catch (const exception& e) {
            cout << "[ERREUR] " << e.what() << '\n';
        }
    } while (choix != 10);

    return 0;
}
