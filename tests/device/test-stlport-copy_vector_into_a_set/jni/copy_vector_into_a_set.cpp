#include <iostream>
#include <vector>
#include <set>

using namespace std;
struct PasswordFormData {
  const char* signon_realm;
};

struct PasswordForm {
  std::string signon_realm;
  PasswordForm();
  ~PasswordForm();
};


PasswordForm::PasswordForm(){
}

PasswordForm::~PasswordForm() {
}

PasswordForm* CreatePasswordFormFromData(
    const PasswordFormData& form_data) {
  PasswordForm* form = new PasswordForm();
  if (form_data.signon_realm) {
    form->signon_realm = "http://foo.example.com";
  }
  return form;
}

typedef std::set<const PasswordForm*> SetOfForms;

int main()
{
  static const PasswordFormData form_data[] = {
      { "http://foo.example.com"},
    };
  std::vector<PasswordForm*> vector1;
  for (int i = 0; i < sizeof(form_data)/sizeof(form_data[0]); ++i) {
    PasswordForm* form = CreatePasswordFormFromData(form_data[i]);
    vector1.push_back(form);
  }

#if BUG
  SetOfForms set1(vector1.begin(), vector1.end());
#else
  SetOfForms set1;
  std::copy(vector1.begin(), vector1.end(), std::inserter(set1, set1.begin()));
#endif   
  for(std::vector<PasswordForm*>::iterator iter = vector1.begin(); iter != vector1.end(); ++iter) {
    const PasswordForm* value = *iter;
    cout << "************************************************ vector value " << value->signon_realm.c_str() << std::endl;
  }
  cout<<"******************************************** set size "<< set1.size() << std::endl;
  for(SetOfForms::iterator iter = set1.begin(); iter != set1.end(); ++iter) {
    const PasswordForm* value = *iter;
    cout << "************************************************ set value " << value << std::endl;
    cout << "************************************************ set value " << value->signon_realm.c_str() << std::endl;
  }
    return 0;
}
