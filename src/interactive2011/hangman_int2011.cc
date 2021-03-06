/*
 * hangman_int2011.cc
 *
 */

#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/assign/list_of.hpp>

#include "hangman_int2011.h"

Int2011::Int2011(IOEventParser& my_iep, const std::string& path_to_mapping_file, SoundsUtil* my_su, const std::vector<std::string> my_alph, const std::vector<std::string> wordlist) : 
  IBTApp(my_iep, path_to_mapping_file), iep(my_iep), su(my_su), alphabet(my_alph), int2011_words(wordlist), firsttime(true), turncount(0), mistake(0), correctcount(0), j(0), word(""), target_letter(""), word_pos(0), word_length(4), answer(""), button_zero_down(false)
{

  for(size_t i = 0; i < int2011_words.size(); i++) {
    choices.push_back(int2011_words[i]);
  }

  //HM_new();
}

Int2011::~Int2011() {
}

void Int2011::processEvent(IOEvent& e) {

  // check for reset
  // first check when user presses button zero
  if (e.type == IOEvent::BUTTON_DOWN && e.button == 0) {
    button_zero_down = true;
  }
  // now check for when user releases button zero
  if (e.type == IOEvent::BUTTON_UP && e.button == 0) {
    button_zero_down = false;
  }
  // finally if another button event happens before the button up then reset
  if (button_zero_down && e.type == IOEvent::BUTTON_DOWN && e.button != 0) {
    button_zero_down = false;
    firsttime = true;
    HM_new(); // strart over
    return;
  }


  //echo back the dots the user is typing
  int dot = getDot(e);
  if( isDown(e) && dot > 0 )
    su->sayNumber(getStudentVoice(), dot, false);

  //Whenever the user hits Button0 we immediately want the LETTER event to be generated so that he doesnt have to wait for the timeout
  if( e.type == IOEvent::BUTTON && e.button == 0 ) {
    iep.flushGlyph();
    return; //required? hmm..
  }

  if( e.type == IOEvent::CELL_LETTER || e.type == IOEvent::BUTTON_LETTER ) {
    printEvent(e);
    //Upon entering this mode, we dont want any pending LETTER events to interfere. So we skip the first LETTER event.
    if( firsttime )//Check if this is the first letter event, if so, we skip it
    {
      std::cout << "    (DEBUG)Skipping first letter event" << std::endl;
      firsttime = false;
      return;//skip
    }
    else
    {
      su->sayLetter(getStudentVoice(), (std::string) e.letter);
      HM_attempt((std::string) e.letter);
    }
  }
}

void Int2011::HM_new() {
  static const Charset &charset = IBTApp::getCurrentCharset();

  turncount = 0;
  mistake = 0;
  correctcount = 0;
 
  int s; //number of dashes
  
  random_shuffle(choices.begin(), choices.end());
  word = choices.front();
  word_pos = 0;

  su->saySound(getTeacherVoice(), "interactive 2011 welcome");
  su->saySound(getTeacherVoice(), "word 4 letters");

  for(s = 0; s < word_length; s++) {
    su->saySound(getTeacherVoice(), "DASH");
  }
  int num_bytes_in_letter = numBytesInUTF8Letter(word);
  answer.clear();

  //We want answer's size to be same as word's.
  answer.resize(word.size());// filled with null characters.
  
  //Fill answer with *s, but add these *s every num_bytes_in_letter intervals. To keep consistent with word's
  for(size_t i = 0; i<word.size();) {
    answer.replace(i,1,"*");
    i = i + numBytesInUTF8Letter(word.at(i));
  }
  
  //answer = std::string(word_length, '*');//answer is originally all  *'s students need to guess what the *'s are
  su->saySound(getTeacherVoice(), "guess a letter"); //will change to please guess
  std::cout << "Current answer is " << word << std::endl;
  std::cout << "Current guessed word is " << answer <<std::endl;
  std::cout<<"		(DEBUG)Answer size is:"<<answer.size()<<std::endl;
}

void Int2011::HM_attempt(std::string i) {

  turncount = 0;
  if( mistake == 7 ) { 
    //after 7 mistakes students give one chance to guess the word
    //int index = alphabeticIndex(alphabet, word, word_pos, word.at(word_pos));
    int num_bytes_in_letter = numBytesInUTF8Letter(word.at(word_pos));
    std::string correct_letter(word, word_pos, num_bytes_in_letter);
    std::cout << "		(DEBUG)Corr letter:" << correct_letter << " word.at(word_pos)" << correct_letter << std::endl;
    if( i.compare(correct_letter) == 0 ) { //letter is next in sequence
      word_pos = word_pos + numBytesInUTF8Letter(word.at(word_pos)); //move to next character
      if( word_pos == word.size() ) { //are we done?
        su->saySound(getTeacherVoice(), "good");
	su->saySound(getTeacherVoice(), "well done you guessed it");
        std::cout << "WOW you guessed it correctly" << std::endl;
        //HM_new();
        return;
      }
    } else { //guess is incorrect
      su->saySound(getTeacherVoice(), "no");
      su->saySound(getTeacherVoice(), "solution");
      su->sayLetterSequence(getTeacherVoice(), word);
      std::cout << "SORRY you have run out of guesses the correct word was " << word << std::endl;
      //HM_new();
      return;
    }
    // mistake guess method ends here
  } else {
    for(; word_pos < word.size(); word_pos = word_pos + numBytesInUTF8Letter(word.at(word_pos))) {
      int index = alphabeticIndex(alphabet, word, word_pos, word.at(word_pos));
      int num_bytes_in_letter = numBytesInUTF8Letter(word.at(word_pos));
      std::string correct_letter(word, word_pos, num_bytes_in_letter);
      if( i.compare(correct_letter) == 0 ) { //letter is next in sequence
        if( answer.at(word_pos) != '*' ) {
          su->saySound(getTeacherVoice(), "same letter");
          std::cout << "You have already guessed that letter please guess again" << std::endl;
          mistake--;
        } else {
          answer.replace(word_pos, num_bytes_in_letter, correct_letter);
          //std::cout<<"		(DEBUG)num bytes:"<<num_bytes_in_UTF8letter<<" letter is:"<<letter<<" answer is:"<<answer<<" word.at(word_pos) is:"<<word.at(word_pos)<<std::endl;
          turncount++;
          correctcount++;
          //word_pos++; //move to next character
	  su->saySound(getTeacherVoice(), "good guess");
          std::cout << "    (DEBUG)Picked a good letter!" << std::endl;
          std::cout << "    (DEBUG)word_pos:" << word_pos << " word.at(word_pos):" << correct_letter << " Answer is:"<<answer<<std::endl;
        }
      }
    }
    word_pos = 0;
    if( turncount == 0 ) {
      mistake++;
      if( mistake == 7 ) {
        std::cout << "You have run out of guesses but you have one last chance to guess the word." << std::endl;
        std::cout << "Please write your guess" << std::endl;
      }
      su->saySound(getTeacherVoice(), "no");
      std::cout << "Letter guessed is not present please guess another letter" << std::endl;
    }
    if( correctcount == word_length ) {
      std::cout << "GOOD JOB YOU SPELLED " << word << " correctly" << std::endl;
      su->saySound(getTeacherVoice(), "good");
      su->saySound(getTeacherVoice(), "well done you guessed it");
      //HM_new();
      return;
    }
    su->saySound(getTeacherVoice(), "current");
    for(int s = 0; s < answer.size();) {
      int num_bytes_in_letter = numBytesInUTF8Letter(answer.at(s));
      std::string letter(answer, s, num_bytes_in_letter);
      std::cout<<"		(DEBUG)Extracted letter from answer is:"<<letter<<std::endl;
      if( !strcmp(letter.c_str(), "*") ) {
        su->saySound(getTeacherVoice(), "DASH");
      } else {
        su->sayLetter(getTeacherVoice(), letter);
      }
      s=s+num_bytes_in_letter;
    }
    if( mistake == 7 ) {
      su->saySound(getTeacherVoice(), "7 mistakes");
    } else {
      switch( mistake ) {
      case 0:
	su->saySound(getTeacherVoice(), "0 mistakes");
	break;
      case 1:
	su->saySound(getTeacherVoice(), "1 mistake");
	break;
      case 2:
	su->saySound(getTeacherVoice(), "2 mistakes");
	break;
      case 3:
	su->saySound(getTeacherVoice(), "3 mistakes");
	break;
      case 4:
	su->saySound(getTeacherVoice(), "4 mistakes");
	break;
      case 5:
	su->saySound(getTeacherVoice(), "5 mistakes");
	break;
      case 6:
	su->saySound(getTeacherVoice(), "6 mistakes");
	break;
      }

      su->saySound(getTeacherVoice(), "guess a letter");
    }
    std::cout << "Current guessed word is " << answer << std::endl;
    std::cout << "    (DEBUG)word_pos:" << word_pos <<" Answer is:"<<answer<<std::endl;
  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
EnglishInt2011::EnglishInt2011(IOEventParser& my_iep) :
  Int2011(my_iep, "", new EnglishSoundsUtil, createAlphabet(), createWords())
{
}

const std::vector<std::string> EnglishInt2011::createAlphabet() const
{
  return boost::assign::list_of("A")("B")("C")("D")("E")("F")("G")("H")("I")("J")("K")("L")("M")("N")("O")("P")("Q")("R")("S")("T")("U")("V")("W")("X")("Y")("Z");
}

// Interactive 2011 challenge words
const std::vector<std::string> EnglishInt2011::createWords() const
{
  return boost::assign::list_of("BOOK")("NOTE")("GAME")("READ")("PLAY");
}
