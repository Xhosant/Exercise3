
ΕΡΓΑΣΙΑ 3 

a. Σχεδιαστικές επιλογές & παραδοχές

Η εργασία υλοποιεί εξωτερική ταξινόμηση με συγχώνευση (external merge sort) πάνω σε αρχεία τύπου heap, χρησιμοποιώντας το BF επίπεδο διαχείρισης blocks και το HP API.
Η ταξινόμηση γίνεται με κριτήριο (name, surname), όπως ορίζεται από την εκφώνηση.

Κατά τη σχεδίαση έγιναν οι εξής βασικές παραδοχές:

Κάθε run (chunk) αποτελείται από chunkSize blocks και χωράει στη μνήμη.

Κατά τη συγχώνευση χρησιμοποιούνται bWay input chunks και ένα output buffer.

Δεν αλλάζει η δομή του Record ούτε το binary layout των εγγραφών.

Κάθε block γίνεται pin/unpin ακριβώς μία φορά ανά χρήση.

Χρησιμοποιήθηκε το γλωσσικό μοντέλο ChatGPT αποκλειστικά ως βοήθημα για:

κατανόηση της θεωρίας της external merge sort,

debugging (Makefile, linker flags, BF usage),

σχεδιασμό πειραματικών mains για μέτρηση merge passes.

Η τελική υλοποίηση, οι πειραματισμοί και η επαλήθευση της ταξινόμησης έγιναν από τον φοιτητή.

b. Παρατηρηθείσες δυσλειτουργίες / περιορισμοί


Η απόδοση εξαρτάται έντονα από τις παραμέτρους chunkSize και bWay.

Δεν παρατηρήθηκαν σφάλματα ορθότητας στην τελική έκδοση. Η καθολική ταξινόμηση επαληθεύεται με ειδική main.

c. Συνοπτική αναφορά αλλαγών ανά συνάρτηση

shouldSwap(Record r1, Record r2)
Υλοποιεί τη σύγκριση εγγραφών βάσει (name, surname). Χρησιμοποιείται τόσο στην ταξινόμηση των chunks όσο και στην επαλήθευση της καθολικής ταξινόμησης.

sort_FileInChunks(int inputFd, int chunkSize)
Διαβάζει το αρχείο εισόδου σε διαδοχικά chunks μεγέθους chunkSize blocks.
Για κάθε chunk φορτώνονται τα blocks στη μνήμη, δημιουργείται CHUNK δομή και καλείται η sort_Chunk.

sort_Chunk(CHUNK *chunk)
Ταξινομεί τις εγγραφές ενός chunk που βρίσκεται πλήρως στη μνήμη.
Χρησιμοποιείται comb sort πάνω σε array εγγραφών.

CHUNK_Iterator / CHUNK_RecordIterator
Υλοποιούν επανάληψη σε chunks, blocks και εγγραφές.
Εξασφαλίζεται ότι κάθε block γίνεται BF_GetBlock και BF_UnpinBlock ακριβώς μία φορά.

CHUNK_GetNextRecord(CHUNK_RecordIterator *it, Record *rec)
Επιστρέφει την επόμενη εγγραφή ενός chunk, διαχειριζόμενη τη μετάβαση μεταξύ records και blocks και τον τερματισμό του chunk.

merge(int inputFd, int chunkSize, int bWay, int outputFd)
Υλοποιεί b-way συγχώνευση.
Συγχωνεύει έως bWay chunks κάθε φορά, παράγοντας νέο αρχείο ανά πέρασμα μέχρι να προκύψει καθολικά ταξινομημένο αρχείο.


Πειραματικές mains

Προστέθηκαν επιπλέον mains:

experiment_main: σύγκριση διαφορετικών bWay.

matrix_main: πίνακας merge passes για διαφορετικά chunkSize και bWay.

scaling_main: επίδραση του αριθμού εγγραφών.

verify_main: επαλήθευση καθολικής ταξινόμησης.

Μεταγλώττιση & Εκτέλεση
Όλες οι εντολές εκτελούνται από τον φάκελο Exercise3-main.
make clean
Βασική ταξινόμηση:
make sort && ./build/sort_main
Πείραμα b-way συγχώνευσης:
make experiment && ./build/experiment_main
Πίνακας πειραμάτων:
make matrix && ./build/matrix_main
Πείραμα κλιμάκωσης:
make scaling && ./build/scaling_main
Επαλήθευση καθολικής ταξινόμησης:
make verify && ./build/verify_main
